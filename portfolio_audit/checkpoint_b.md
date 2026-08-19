# CHECKPOINT B

Commit: `108c4fc68c90e41723fe2ce35d6cb4b443524ed2`

Source modification: none. Final `sha256sum -c portfolio_audit/source_sha256.txt` passed for the Makefile, six production C files, and four headers; `git diff` for existing project files was empty.

## 1. Baseline result

| PASS | PARTIAL | FAIL | CRASH | HARNESS_ERROR | Total |
|---:|---:|---:|---:|---:|---:|
| 16 | 4 | 2 | 1 | 0 | 23 |

Harness mutation validation: 5 PASS / 0 FAIL.

Raw and structured results: `test_results.md`, `test_results.json`, `raw/`, `harness_validation.md`.

## 2. Most important PASS evidence

- **Concurrent creation (B08):** two simultaneously launched teams converged on one SHM/semaphore/queue set, one A cell, one B cell, and an unlocked semaphore at a stable observation point.
- **Start barrier (B10/B11):** three players remained in RECRUITMENT; the fourth produced START_GAME with `[2,2]` team counts and four occupied cells.
- **Cooperative shutdown (B14):** SIGINT to one of four active players propagated END_GAME; all four exited 0 and all IPC objects disappeared.
- **Semaphore owner death (B16):** killing the audit lock owner changed semaphore value from locked 0 back to 1 through `SEM_UNDO`.
- **Semaphore stress (B22):** five helper processes converged on exactly 1000 protected increments and left semaphore value 1.
- **Core algorithm (B20):** nine deterministic probes passed distance boundary, greedy tie/x movement, collision, legal random step, surrounded predicate, death update, and out-of-radius fallback.

## 3. Most important FAIL/PARTIAL/CRASH evidence

1. **F01 / B13 — CRASH:** viewer and IPC remained in 1 of 8 graceful shutdown iterations because viewer attachment could make the last player skip its only cleanup attempt.
2. **F02 / B17-B19 — PARTIAL:** SIGKILL and SIGTERM left all IPC plus a ghost player; a new process joined that stale state, producing two shared players with one live process.
3. **F03 / B05 — FAIL:** empty team string was accepted as team 0 and created a game.
4. **F04 / B06 — FAIL:** `viewmode-extra` entered viewer mode due prefix comparison.
5. **F05 / B21 — PARTIAL:** DEATH kind was consumed as an ordinary chase target; intended semantics are unknown.

Detailed expected/actual/code path/root cause: `failures.md`.

## 4. Correctness versus scope

- Correctness/liveness: viewer cleanup race, empty argument, viewer prefix dispatch.
- Out-of-scope robustness: SIGTERM/SIGKILL state reconciliation. The user confirmed only cooperative SIGINT was intended.
- PARTIAL correctness: DEATH should trigger a game-end check; a general end check exists, but the message is also converted into a chase target.

## 5. Highest-value portfolio failure

**Recommended: F01 viewer/last-player cleanup race.**

Why it is strongest:

- it is a genuine multi-process lifecycle race, not a formatting issue;
- it occurs in the README-recommended viewer-first flow;
- it has direct kernel evidence (persistent SHM/semaphore/queue), process evidence (viewer remains alive), and a short code path;
- it exposes the distinction between kernel attachment count and application-level ownership;
- it supports a measurable Before/After: 8-iteration cleanup stress, leftover IPC count, and bounded viewer exit.

Second candidate: F05 DEATH dispatch. F02 abnormal-exit ghost ownership is excluded from original-scope improvement priority because the user confirmed SIGINT-only support.

## 6. Possible solution architectures

### A. Player-role lifecycle counter + cleanup owner

Track live players separately from viewer attachments under the semaphore. The last player transitions to END_GAME and marks SHM `IPC_RMID`; viewer attachment no longer blocks resource retirement. A cleanup-owner flag prevents multiple removers.

Trade-off: small data-model/protocol change, but SIGKILL still needs stale-player detection.

### B. Dedicated coordinator process

A coordinator exclusively creates/owns IPC, accepts player/viewer registration, observes client death, controls state transitions, and removes resources after clients exit.

Trade-off: clearest ownership and recovery, but largest architectural change and a new single point whose failure must be handled.

### C. PID/generation registry with startup recovery

Store per-player PID, generation, cell, and optional heartbeat. Under the semaphore, participants prune dead PIDs, reconcile board/counters, and reclaim an unattached stale IPC set on startup.

Trade-off: strongest decentralized crash recovery, but highest shared-state and race complexity.

For a focused first improvement, architecture A is the smallest direct fix for F01; architecture C or B is needed for full F02 recovery.

## 7. User rationale received

The five CHECKPOINT B questions were answered. Canonical classification into verified facts, user rationale, inference, and unknowns is in `design_rationale.md`.

Source changes remain blocked until F01 and a solution architecture are explicitly approved.
