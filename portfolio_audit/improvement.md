# Improvement Candidate Selection

Status: recommendation only. No production source has been modified.

## Selection method

Candidates are ranked qualitatively using the supplied priority model:

```text
Portfolio Value × Correctness Impact × Architectural Interest ÷ Implementation Cost
```

No fabricated numeric score is assigned. “High/medium/low” is justified from measured behavior, confirmed requirements, and estimated change surface.

| Rank | Candidate | Portfolio value | Correctness impact | Architectural interest | Estimated cost | Decision |
|---:|---|---|---|---|---|---|
| 1 | F01 viewer/last-player cleanup race | High: genuine reproducible IPC race with Before/After | High: violates confirmed auto-exit/cleanup goal | High: application ownership vs kernel attachment lifecycle | Medium | Recommend first |
| 2 | F05 DEATH kind dispatch | Medium: message protocol semantics | Medium: intended end check is conflated with chase | Medium | Low-medium | Follow-up after semantics are fully specified |
| 3 | F03/F04 CLI validation | Low | Low | Low | Low | Bundle only after architectural fix |
| Excluded | F02 SIGTERM/SIGKILL recovery | Potentially high | Outside intended requirement | High | High | Do not prioritize for baseline improvement |

## Recommended improvement

Fix **F01** using **Architecture A: explicit player lifecycle counter and elected cleanup owner**.

### Problem

`shm_nattch` counts both player and viewer mappings. A viewer attachment at the last player's one-time check makes the player skip cleanup permanently, while the viewer waits for that cleanup.

### Proposed decision

Add application-level lifecycle metadata separate from gameplay alive counts:

- registered player-process count;
- shutdown/cleanup-owner flag;
- optional generation/magic for one game instance.

Every accepted player registers once under the semaphore. Every supported exit path, including surrounded death, normal game end, and SIGINT, converges on one lifecycle function that:

1. decrements the registered player count exactly once under the semaphore;
2. elects the process that observes zero as cleanup owner;
3. releases the semaphore;
4. removes the message queue and semaphore and marks SHM `IPC_RMID`;
5. lets viewer observe the removed SHM key and exit automatically.

Viewer attachments are intentionally excluded from the player counter. `IPC_RMID` can retire the key while a viewer has a transient attachment; the segment is destroyed after the final detach.

### Why this architecture

- directly fixes the confirmed requirement without adding out-of-scope SIGKILL recovery;
- removes the probabilistic one-second sleep as a correctness mechanism;
- preserves the current decentralized process model;
- creates a deterministic ownership invariant testable without timing luck.

### Trade-offs

- requires a new shared lifecycle field and exact-once deregistration discipline;
- all player exit paths must be unified to prevent counter drift;
- removing the semaphore/queue must occur only after the final player has left their critical sections;
- still does not make SIGKILL recoverable, consistent with stated scope.

## Alternative architectures

### B. Dedicated coordinator

Strong ownership and easier client-death observation, but materially expands the architecture and project scope.

### C. PID/generation registry and reaper

Supports SIGKILL recovery and stale-state repair, but is unnecessary for the stated SIGINT-only requirement and carries the highest concurrency cost.

## Required regression suite after approval

- B13 viewer lifecycle: increase repeated graceful runs and require 0 leftover IPC/viewer timeouts;
- B14 four-player SIGINT propagation: all exit 0 and IPC absent;
- B07/B15 single-player and repeated graceful cleanup;
- B18 remains an explicitly unsupported SIGTERM observation, not a regression gate;
- source hashes and all 23 baseline cases rerun for unintended changes.

## Approval boundary

PHASE 11 source changes require explicit approval of:

1. F01 as the selected problem; and
2. Architecture A as the implementation direction.
