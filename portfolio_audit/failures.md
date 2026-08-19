# Failure Analysis

Commit: `108c4fc68c90e41723fe2ce35d6cb4b443524ed2`

All observed behavior is `VERIFIED FROM RUNTIME TEST`. Root causes are `VERIFIED FROM CODE` where the complete path is cited. Design intent supplied after CHECKPOINT B is labeled `USER-PROVIDED RATIONALE`.

## F01 — Viewer can prevent the last-player cleanup forever

Classification: **CRASH** — resource-lifecycle race leading to an indefinitely running viewer and persistent IPC.

Baseline case: B13. Eight viewer + one-player graceful-shutdown iterations rendered correctly; one iteration left all three IPC objects and the viewer alive after the player exited 0.

### Expected

README's recommended flow starts viewer mode before the players (`README.md:68-89`). SIGINT sets `END_GAME`; the last player removes IPC; viewer observes missing SHM and exits.

`USER-PROVIDED RATIONALE`: viewer is intentionally expected to remain open throughout the game, automatically exit when the game ends, and participate in complete resource reclamation.

### Actual

In iteration 8/8:

- player received SIGINT and exited 0;
- SHM, one semaphore set, and message queue remained;
- shared state was `END_GAME` with one stale player/cell;
- viewer remained alive and required harness SIGKILL;
- harness then removed the three IPC objects.

Raw evidence: `portfolio_audit/raw/B13.log` and `B13.viewer.08.stdout`.

### Relevant code path

1. Viewer repeatedly calls `shmget`, attaches read-only, renders, and detaches (`src/utils.c:44-86`) every 16,667 μs (`src/main.c:34-38`).
2. Viewer exits only after the SHM lookup starts returning `-1` (`src/utils.c:53-62`).
3. A terminating player detaches both mappings, sleeps one second, samples `shm_nattch` once, and removes IPC only when the sampled count is zero (`src/game_algorithm.c:343-358`).

### Root cause

`VERIFIED FROM CODE`: the transient kernel SHM attachment count is used as the cleanup-ownership decision, but viewer attachments are included in that count. If the viewer is attached at the last player's single check:

```text
viewer attached
    ↓
last player sees shm_nattch > 0
    ↓
last player skips clear_ipcs and exits
    ↓
viewer waits for SHM removal
    ↓
no process remains that can remove SHM/semaphore/queue
```

This is a circular liveness dependency. The one-second sleep changes probability but does not establish ownership or exclusion.

### Severity

High for lifecycle correctness and repeatability. It affects the documented viewer-first execution flow and leaves kernel resources after a graceful player shutdown.

### Possible fix directions

- elect an explicit cleanup owner and call `IPC_RMID` based on player lifecycle, not total SHM attachments;
- track player and viewer roles separately in shared metadata, with the last **player** initiating cleanup;
- move IPC ownership to a coordinator process that outlives players and controls shutdown.

No fix has been implemented.

## F02 — SIGTERM/SIGKILL leave ghost players and contaminate the next run

Classification: **PARTIAL / OUT OF SUPPORTED SCOPE** — measured robustness limitation. Semaphore ownership recovers, but logical state and IPC lifecycle do not.

Baseline cases: B16 PASS for `SEM_UNDO`; B17/B18/B19 PARTIAL for SIGKILL, SIGTERM, and restart.

### Expected

`USER-PROVIDED RATIONALE`: only cooperative SIGINT shutdown was intended; SIGTERM/SIGKILL recovery was not a requirement. Therefore these cases characterize unsupported robustness rather than a baseline requirement failure. `SEM_UNDO` progress remains a useful verified property.

### Actual

- SIGKILL: process exit `-9`; `nattch` fell to 0 and semaphore value remained 1, but SHM/semaphore/queue, `player_nbs=1`, `team_nbs=[1,0]`, one A cell, and three queued positions remained.
- SIGTERM: same persistent state with exit `-15`.
- Restart: a new single live process joined the stale run; shared state became two A players/two A cells while the harness observed only one live target process.

### Relevant code path

- only SIGINT is registered (`src/main.c:9-20`);
- SIGINT uses the cooperative END_GAME path (`src/main.c:24-29`);
- `SEM_UNDO` repairs semaphore adjustment when a process dies (`src/system.c:61-77`);
- board/counters contain no PID, generation, lease, or heartbeat (`include/data.h:38-65`);
- cleanup is executed only through `terminate_player` (`src/game_algorithm.c:343-358`).

### Root cause

`VERIFIED FROM CODE`: the kernel can automatically detach SHM and undo semaphore operations, but it cannot infer which board cell/counters belong to the dead process. The application has no ownership metadata or recovery coordinator, and normal termination code is bypassed by default SIGTERM and unavoidable SIGKILL.

### Severity

Potentially high operational impact outside the supported scope, but it is not prioritized as an original-requirement correctness bug. Correctness of the intended cooperative SIGINT path remains separately verified by B14/B15.

### Possible fix directions

- add SIGTERM to the cooperative signal path for catchable termination;
- add per-player PID/generation/heartbeat metadata and a reaper that clears dead ownership under the semaphore;
- on startup, detect `shm_nattch == 0` with stale state and atomically reclaim/reinitialize the whole IPC set;
- use a coordinator that owns all resources and detects client death.

No fix has been implemented.

## F03 — Empty team argument becomes team 0

Classification: **FAIL** — input correctness bug.

Baseline case: B05.

### Expected

`./lemipc ""` is not a numeric team identifier and should exit nonzero without creating IPC.

### Actual

The process remained alive, created all three IPC objects, and joined as team A with `players=1`, `teams=[1,0]`, and one A cell.

### Root cause

The digit loop executes zero times for an empty string, then `ft_atoi("")` yields 0 and the range check accepts it (`src/utils.c:105-121`).

### Severity

Low operational severity; deterministic CLI contract violation.

### Possible fix direction

Reject `argv[1][0] == '\0'` before the digit loop. No fix has been implemented.

## F04 — Any string beginning with `viewmode` enters viewer mode

Classification: **FAIL** — input/dispatch correctness bug.

Baseline case: B06.

### Expected

Only the documented exact token `viewmode` selects viewer mode.

### Actual

`viewmode-extra` entered the indefinite viewer loop and had to be interrupted; exit status was signal-derived `-2`.

### Root cause

Dispatch compares only the first eight bytes with `ft_strncmp(argv[1], "viewmode", 8)` and does not require a terminating NUL (`src/main.c:34`).

### Severity

Low operational severity; deterministic dispatch violation.

### Possible fix direction

Require exact string equality or verify `argv[1][8] == '\0'`. No fix has been implemented.

## F05 — `DEATH` kind has no distinct receive-side semantics

Classification: **PARTIAL** — intended terminal check exists globally, but the message kind has no distinct branch and also creates a chase side effect.

Baseline case: B21 direct algorithm probe.

### Expected

The enum and sender distinguish `DEATH` from chase/BROADCAST (`include/data.h:11-18`, `src/game_algorithm.c:159-182`).

`USER-PROVIDED RATIONALE`: when a teammate receives `DEATH`, it should check whether the game has ended.

### Actual

A `DEATH` message with an in-radius coordinate set `track_flag=1` and became target `[5,6]`, exactly like an ordinary chase coordinate. The same loop subsequently executes the general `check_alone_on_board` end check, so end detection is not entirely absent (`src/game_algorithm.c:324-333`).

### Root cause

`run_swarm_intelligence` checks only receive success and distance; it does not branch on `kind`, `team_no`, or `msg_order` (`src/game_algorithm.c:197-218`). The intended game-end check occurs later for every loop iteration, not as DEATH-specific dispatch, and the earlier chase-target mutation remains.

### Severity

Medium correctness significance: the game-end check is present, but DEATH also mutates tracking state contrary to the stated single purpose unless that extra behavior was separately intended. That extra intent remains `UNKNOWN`.

### Possible fix directions

- dispatch explicitly by `kind`, making DEATH perform/check terminal-state handling without setting a chase target;
- document what happens when the game has not ended, then add a deterministic DEATH regression test.

No fix has been implemented.

## Correctness bugs versus scope limitations

| Finding | Current interpretation | Basis |
|---|---|---|
| F01 viewer/cleanup race | Correctness + liveness bug | Violates documented viewer-first graceful lifecycle; runtime CRASH |
| F02 abnormal-exit stale state | Out-of-scope robustness limitation | User states only SIGINT shutdown was supported |
| F03 empty argument | Correctness bug | Invalid argument changes system state |
| F04 prefix viewer dispatch | Correctness bug | Non-exact argument enters indefinite mode |
| F05 DEATH conflation | PARTIAL correctness issue | Intended end check occurs globally, but DEATH also becomes a chase target |
