# Design Rationale

Recorded after CHECKPOINT B on 2026-08-19. This document deliberately separates runtime/code facts from the user's design rationale and audit inference.

## Verified facts

### VERIFIED FROM CODE

- Viewer repeatedly attaches read-only, renders, and detaches; it exits only after SHM lookup fails (`src/utils.c:44-86`).
- Each player keeps one read-only and one writable mapping to the same SHM segment (`src/logic.c:92-101`).
- Graceful cleanup samples `shm_nattch` once after a one-second sleep and removes all IPC only when the value is zero (`src/game_algorithm.c:343-358`).
- Only SIGINT has an installed cooperative signal handler (`src/main.c:9-29`).
- One global semaphore serializes the gameplay critical section (`src/system.c:61-77`, `src/game_algorithm.c:324-335`).
- One message queue multiplexes initialization, start, and team channels using `mtype` (`include/data.h:45-52`).
- `DEATH` is emitted as a distinct kind, but the receive-side swarm path does not branch on kind (`src/game_algorithm.c:159-218`).
- The gameplay loop performs `check_alone_on_board` after receiving a team message (`src/game_algorithm.c:324-333`).

### VERIFIED FROM RUNTIME TEST

- Viewer/last-player cleanup left all IPC and viewer alive in 1 of 8 repeated graceful shutdowns (B13 CRASH).
- Four-player cooperative SIGINT shutdown removed IPC and all four processes exited 0 (B14 PASS).
- SIGKILL/SIGTERM bypassed logical cleanup and left a ghost player, while `SEM_UNDO` restored semaphore progress (B16-B19).
- An in-radius DEATH message set a chase target before the loop's general end check (B21 PARTIAL).

## User-provided rationale

### Viewer lifecycle

`USER-PROVIDED RATIONALE`: viewer is intended to run for the full game, terminate automatically when the game ends, and allow/participate in automatic resource reclamation.

### Supported shutdown

`USER-PROVIDED RATIONALE`: only SIGINT shutdown was intended. SIGTERM/SIGKILL recovery was not part of the requirement.

### DEATH message

`USER-PROVIDED RATIONALE`: when a teammate receives DEATH, it should check whether the game has ended.

### Shared memory and cleanup choice

`USER-PROVIDED RATIONALE`: shared memory was selected so processes could inspect shared content and communicate, and `shm_nattch`-based cleanup was selected to automate resource reclamation.

### Synchronization simplification

`USER-PROVIDED RATIONALE`: substantial design effort was spent preventing deadlock and data races and simplifying that design; this motivated the single global semaphore and typed queue approach.

## Inference

- `INFERENCE`: F01 is an original-requirement correctness/liveness bug, because its runtime behavior directly violates the confirmed viewer auto-exit/resource-reclamation goal.
- `INFERENCE`: using one global semaphore concentrates all shared gameplay state under one lock to reduce lock ordering and data-race complexity. The likely trade-off is serialized access, but performance was not measured and was not stated as a user rationale.
- `INFERENCE`: `shm_nattch` was treated as a proxy for “last application participant,” but kernel attachment count includes viewers and cannot distinguish player ownership.
- `INFERENCE`: the current DEATH path partially meets the intent because every loop checks whether one team remains, but converting DEATH coordinates into a chase target is an additional behavior not justified by the supplied rationale.
- `INFERENCE`: the two mappings express read-oriented versus write-oriented access in the local API, but both mappings still affect `shm_nattch` and therefore lifecycle decisions.

## Unknown

- Why two simultaneous mappings were preferred over one writable mapping plus API-level const/read discipline is `UNKNOWN`.
- Which alternative synchronization architectures were considered is `UNKNOWN`.
- Which specific subsystem was simplified, beyond the general deadlock/data-race goal, is `UNKNOWN`.
- Whether the single global semaphore's serialization cost was consciously accepted is `UNKNOWN`; no performance benchmark exists.
- After a DEATH message when the game is not over, the exact intended next action is `UNKNOWN`.
- What the user would change if redesigning the project today is `UNKNOWN`.

These unknowns do not block selection of the viewer lifecycle race as the first improvement candidate.
