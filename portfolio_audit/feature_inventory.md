# Feature Inventory

Status in this table means **code implementation coverage**, not runtime correctness.

- `IMPLEMENTED`: a complete-looking source path exists for the stated narrow behavior.
- `PARTIAL`: a source path exists but an observable edge, error path, or stated sub-behavior is absent.
- `NOT IMPLEMENTED`: no source path exists for the stated behavior.
- `UNKNOWN`: only execution can establish the property.

The runtime column below is the frozen CHECKPOINT A state. Subsequent evidence is recorded by case ID in `test_results.md` and summarized in `checkpoint_b.md`.

| Feature | Status | Source Evidence | Runtime Verified at CHECKPOINT A? |
|---|---|---|---|
| Team selection restricted to numeric range 0..1 | PARTIAL | `src/utils.c:105-121` checks characters and range, but an empty string reaches `ft_atoi` and is not explicitly rejected | No |
| Viewer-mode dispatch | PARTIAL | `src/main.c:34-40`; first 8 characters are compared, so a longer string with prefix `viewmode` is accepted | No |
| Viewer attach/render/detach lifecycle | IMPLEMENTED | `src/utils.c:44-86` | No |
| First-player election by exclusive SHM creation | IMPLEMENTED | `src/system.c:13-19,43-46` | No |
| Creation/reuse of one SHM, semaphore, and message queue | IMPLEMENTED | `src/system.c:6-40` | No |
| Semaphore initialization to 1 by the elected first process | IMPLEMENTED | `src/system.c:26-39` | No |
| Shared-state initialization to recruitment, empty board, zero team counts | PARTIAL | `src/player.c:7-11` initializes state/board/team counts; it relies on new SHM zero-fill for `player_nbs` rather than assigning it | No |
| Four unique candidate start positions from a 100-cell permutation | IMPLEMENTED | `src/logic.c:76-90`, `src/player.c:12-23` | No |
| Team capacity enforcement: maximum two players | IMPLEMENTED | `src/logic.c:47-59` | No |
| Admission only during RECRUITMENT | IMPLEMENTED | `src/logic.c:50-57` | No |
| Global and per-team count update on admission | IMPLEMENTED | `src/logic.c:50-53` | No |
| Initial-position delivery and board placement | PARTIAL | `src/player.c:27-43`; non-`-1` receive outcomes other than success can leave `msgbuf` unchecked/uninitialized | No |
| Four-player start barrier and four release messages | IMPLEMENTED | `src/logic.c:15-30` | No |
| Nonblocking System V message send/receive wrappers | IMPLEMENTED | `src/system.c:105-120` | No |
| Team-isolated message channel by `mtype` | IMPLEMENTED | `src/game_algorithm.c:163,176,189` | No |
| Kind-aware team message semantics | NOT IMPLEMENTED | `src/game_algorithm.c:197-218` uses message coordinates without branching on `kind`, `team_no`, or `msg_order` | No |
| Radius-3 chase-message acceptance | IMPLEMENTED | `src/game_algorithm.c:197-218` | No |
| Dominant-axis greedy chase movement | IMPLEMENTED | `src/game_algorithm.c:106-135` | No |
| Eight-direction random movement fallback | IMPLEMENTED | `src/game_algorithm.c:138-156,274-285` | No |
| Collision notification and temporary hold/random fallback | IMPLEMENTED | `src/game_algorithm.c:236-285,295-307` | No |
| Surrounded-player detection on opposite axes | IMPLEMENTED | `src/game_algorithm.c:51-82` | No |
| Death clears board cell and decrements counts | IMPLEMENTED | `src/game_algorithm.c:220-227` | No |
| One-team-left end detection | IMPLEMENTED | `src/game_algorithm.c:33-49,324-337` | No |
| SIGINT broadcast through shared END_GAME state | IMPLEMENTED | `src/main.c:7-29`, `src/logic.c:69-74` | No |
| Graceful dual-detach and last-attachment IPC removal | PARTIAL | `src/game_algorithm.c:343-358`; removal path exists, but syscall results and `IPC_STAT` failure are not checked | No |
| Lock-owner death recovery | PARTIAL | `src/system.c:61-77` uses `SEM_UNDO`; logical board/count ownership is not repaired | No |
| SIGTERM/SIGKILL player-state recovery | NOT IMPLEMENTED | Only SIGINT is registered (`src/main.c:14-20`); shared model has no PID/owner registry (`include/data.h:38-65`) | No |
| Error propagation across admission and gameplay | PARTIAL | `check_player_nbs` returns `1` on lock errors while caller tests `-1` (`src/logic.c:47-59`, `src/player.c:31`); several gameplay send/detach results are ignored | No |
| Consistent viewer snapshots | PARTIAL | Viewer reads shared fields without the semaphore (`src/utils.c:63-85`) | No |
| Deadlock freedom under concurrent normal operation | UNKNOWN | Requires bounded multi-process runtime tests | No |
| Repeatable cleanup across repeated runs | UNKNOWN | Requires before/after IPC snapshots over repeated runtime tests | No |

## Inventory interpretation

The table separates the existence of a function or enum from semantic support. In particular, a `DEATH` value is emitted, but the team receive path does not implement distinct death-message behavior; therefore enum/table presence is not counted as kind-aware semantic support.
