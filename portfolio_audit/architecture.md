# Architecture and Functional Scope

Unless explicitly marked otherwise, statements in this document are `VERIFIED FROM CODE`. No runtime correctness is claimed at CHECKPOINT A.

## One-line result

`lemipc` is a native C multi-process board simulation in which independent player processes coordinate a shared 10×10 game state through one System V shared-memory segment, one semaphore, and one message queue.

This is a code-verified description, not a runtime-verified result.

## Processing architecture

```text
CLI: team 0/1                                      CLI: viewmode
      |                                                   |
      v                                                   v
argument validation                               ftok + shm lookup loop
      |                                                   |
      v                                                   v
ftok("/tmp", 'A')                                read-only attach/snapshot
      |                                                   |
      v                                                   v
exclusive SHM creation elects first process       ANSI board/count output
      |                                                   |
      v                                                   v
create/reuse SHM + 1 semaphore + message queue     detach; stop after SHM removal
      |
      +-- first process: semaphore=1, state=RECRUITMENT,
      |                  clear board/counts, enqueue 4 unique positions
      |
      v
attach same SHM read-only + read-write
      |
      v
locked team-capacity/state check and count increment
      |
      v
receive initial position -> locked board placement
      |
      +-- fourth admitted player: state=START_GAME,
      |                            enqueue 4 start messages
      |
      v
wait for START_GAME message
      |
      v
receive one team message (nonblocking)
      |
      v
global semaphore critical section
      |
      +-- <=1 live team -> terminate
      +-- surrounded -> clear cell/decrement counts
      +-- otherwise -> greedy or random movement
      |
      v
unlock -> optional team message -> 100 ms loop
      |
      v
detach two SHM mappings; after 1 s, last attachment removes all IPC objects
```

Primary source paths: `src/main.c:32-74`, `src/system.c:6-120`, `src/player.c:5-54`, `src/logic.c:5-125`, and `src/game_algorithm.c:197-358`.

## Components and responsibilities

| Component | Responsibility | Source evidence |
|---|---|---|
| Entry/dispatch | Viewer versus player dispatch, signal registration, start-message wait, game-loop entry | `src/main.c:7-74` |
| IPC adapter | Key generation; SHM/semaphore/queue creation; lock/unlock; attach/detach; send/receive; remove | `src/system.c:6-120` |
| Recruitment/state logic | First-step orchestration, capacity counters, state changes, random position permutation, board update | `src/logic.c:5-125` |
| Player initialization | First-process shared-state initialization, four initial-position messages, per-player attach/admission/placement | `src/player.c:5-54` |
| Gameplay | Team-message intake, chase/random policy, collision, surrounded death, end detection, process termination | `src/game_algorithm.c:7-358` |
| Viewer/CLI utilities | Board rendering, team color mapping, argument validation and diagnostics | `src/utils.c:28-134` |
| Shared data model | Board, global/team counts, game state, message schema, per-process state | `include/data.h:4-70`, `include/game_algorithm.h:6-15` |

## Data model and channels

### Shared memory

`t_shm` stores:

- `game_state`
- global `player_nbs`
- `board[10][10]`
- two per-team counters in `team_nbs[2]`

Every player attaches the same segment twice: one `SHM_RDONLY` mapping and one writable mapping (`src/logic.c:92-101`, `src/system.c:79-95`). Player position, IPC IDs, and local algorithm flags remain process-local (`include/data.h:54-65`, `include/game_algorithm.h:6-15`). There is no shared PID/player registry.

### Semaphore

One semaphore protects shared mutations and most gameplay inspection/mutation as a single global critical section. Both lock and unlock use `SEM_UNDO` (`src/system.c:61-77`). Game-state polling in `src/main.c:60` and `src/game_algorithm.c:320`, and viewer snapshots in `src/utils.c:63-85`, do not acquire this semaphore.

### Message queue

The queue uses `mtype` as a channel selector:

- `BROADCAST` (`10`) for four initial positions
- `START_GAME` (`13`) for the four-player start release
- `'A'`/`'B'` for team messages

Messages contain coordinates, kind, team, and order (`include/data.h:45-52`). Initial and start receivers inspect `kind`; the game-loop team receiver consumes a message without kind-specific dispatch and treats its coordinates as a possible chase target (`src/player.c:32-42`, `src/main.c:64-70`, `src/game_algorithm.c:185-218`).

## Control and resource lifecycle

1. `shmget(... IPC_CREAT | IPC_EXCL)` elects the first process (`src/system.c:43-46`).
2. The first process initializes semaphore value 1 and initializes shared state (`src/system.c:26-39`, `src/player.c:5-24`).
3. Each accepted process increments counts under the semaphore, then places its board marker in a separate critical section (`src/logic.c:47-59`, `src/player.c:27-43`, `src/logic.c:104-108`).
4. Gameplay checks and board/count changes occur inside the global semaphore (`src/game_algorithm.c:324-335`).
5. A surrounded player clears its cell and decrements both counters; a winning survivor detaches without decrementing because the game is ending (`src/game_algorithm.c:220-227`, `src/game_algorithm.c:288-304`).
6. Graceful termination detaches both mappings, sleeps one second, checks `shm_nattch`, and removes SHM/semaphore/queue only if the attachment count is zero (`src/game_algorithm.c:343-358`).
7. SIGINT sets shared state to `END_GAME` before local termination (`src/main.c:7-29`, `src/logic.c:69-74`). Other fatal/termination signals have no registered recovery path.

## Functional scope

These numbers describe coded scope, not proven semantic correctness.

| Dimension | Code-derived scope | Evidence / limitation |
|---|---:|---|
| Board | 10×10 = 100 cells | `include/data.h:4-5`, `include/data.h:41` |
| Teams | 2 active teams | `MAXTEAM 2`, `include/data.h:6` |
| Capacity | 2 per team; 4 total | `include/data.h:7`, `src/logic.c:17-18`, `src/logic.c:50-53` |
| IPC object classes | 3: SHM, semaphore, message queue | `src/system.c:13-24` |
| Semaphore count | 1 global semaphore | `semget(key, 1, ...)`, `src/system.c:20` |
| Coded game states | 3 used as states: RECRUITMENT, START_GAME, END_GAME | `src/player.c:8`, `src/logic.c:64,71` |
| Emitted message kinds | 4: INIT_PLAYER, START_GAME, BROADCAST/chase, DEATH | `src/player.c:20`, `src/logic.c:24`, `src/game_algorithm.c:166,179` |
| Kind-specific team receive branches | 0 | Team receiver uses coordinates regardless of `kind`, `src/game_algorithm.c:197-218` |
| Random movement candidates | 8 adjacent directions | `src/game_algorithm.c:138-156` |
| Greedy movement candidates | 4 cardinal directions; one dominant-axis step | `src/game_algorithm.c:106-135` |
| Team-message acceptance radius | squared distance ≤ 9 (radius 3) | `src/game_algorithm.c:203-214` |
| Gameplay cadence | nominal 100 ms per loop after work | `src/game_algorithm.c:338` |
| Collision pause policy | hold 2 ticks, force random for 1 tick | `src/game_algorithm.c:257-264,277-283` |
| Explicitly handled signal | SIGINT only | `src/main.c:9-20` |
| Viewer cadence | nominal 16,667 μs | `src/main.c:34-38` |

## Important design characteristics

1. **Single-lock shared-state model.** One semaphore serializes gameplay inspection and mutation. `INFERENCE`: this favors simple cross-field consistency over concurrency, while making semaphore progress a system-wide liveness dependency.
2. **Shared-state minimum, local-agent state.** The segment stores the board and aggregate counts; target tracking and positions also exist locally per process. There is no shared ownership/PID registry. `INFERENCE`: reconstruction after an ungraceful player death is not directly supported by the data model.
3. **First-process election through SHM exclusivity.** Shared-memory creation determines who initializes all three IPC objects. The semaphore and queue themselves are created without `IPC_EXCL`.
4. **Typed queue as multiple logical channels.** A single queue carries initialization, start release, and team traffic, distinguished by `mtype` and sometimes by `kind`.
5. **Two SHM attachments per player.** Code separates read-oriented and write-oriented pointers, but both refer to the same underlying segment. Cleanup correctness depends on detaching both mappings.
6. **Last-attachment cleanup heuristic.** Every graceful terminator may check `shm_nattch`; a one-second delay is used before the check. There is no elected cleanup owner.
7. **Fixed global key.** `ftok("/tmp", 'A')` gives every run in the same IPC namespace the same key. Tests must serialize runs and distinguish pre-existing resources from target-created resources.
8. **Native ABI coupling.** Shared memory and message payloads use native C `int`, enum, and `long` layout with no version/magic field. This is sufficient for identical local binaries, but cross-build/layout compatibility is not represented.
9. **Time-seeded nondeterminism.** Initial placement and movement are seeded from seconds-resolution time plus small per-player values. Exact coordinates are not a stable baseline oracle.

These characteristics identify test targets; they are not runtime failure claims.
