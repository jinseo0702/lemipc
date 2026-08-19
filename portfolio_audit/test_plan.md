# Baseline Test Plan

This is a CHECKPOINT A design only. No harness, fixture, IPC object, process, or runtime result has been created yet.

## Strategy

The baseline will use small black-box multi-process scenarios plus a separate observer/controller under `portfolio_audit/`. The controller will use the public shared-memory/message layouts to inspect stable points and create controlled fixtures without editing production source.

Tests will run serially because all target processes derive the same key from `ftok("/tmp", 'A')`. Every case will record:

1. pre-test SHM/semaphore/message-queue snapshot,
2. exact command and environment,
3. spawned PIDs and bounded deadline,
4. stdout, stderr, exit status or terminating signal,
5. shared state and IPC metadata at defined observation points,
6. post-test IPC snapshot and cleanup attribution.

Harness/setup failure will be reported separately from target PASS/PARTIAL/FAIL/CRASH.

## Source-derived invariants

| ID | Invariant / observation rule | Evidence | Status before runtime |
|---|---|---|---|
| I1 | Every occupied board coordinate remains within the 10×10 array; a move changes at most one step | `include/data.h:4-5,41`, `src/game_algorithm.c:85-103,138-156` | VERIFIED FROM CODE as intended rule |
| I2 | Stable active-state counts satisfy `player_nbs = team_nbs[0] + team_nbs[1]` and each team count is 0..2 | `src/logic.c:47-53`, `src/game_algorithm.c:220-227` | INFERENCE; runtime required |
| I3 | At a stable point, number of `A`/`B` cells equals the corresponding team counts and no two players occupy one cell | locked placement/movement/death paths | INFERENCE; runtime required |
| I4 | Game-state progression is RECRUITMENT → START_GAME → END_GAME, with no normal reverse transition | `src/player.c:8`, `src/logic.c:62-73` | VERIFIED FROM CODE as coded transitions |
| I5 | START_GAME is reached only when total admitted players is 4 | `src/logic.c:15-30` | VERIFIED FROM CODE as intended guard |
| I6 | A third player on one team and any post-start admission do not alter counts or board | `src/logic.c:47-59` | VERIFIED FROM CODE as intended guard |
| I7 | Initial accepted positions are unique and in bounds | shuffled 100-position array, first four queued (`src/logic.c:76-90`, `src/player.c:12-23`) | VERIFIED FROM CODE as construction; runtime required |
| I8 | Shared gameplay inspection/mutation is serialized by the single semaphore | `src/game_algorithm.c:324-335` | VERIFIED FROM CODE for that region |
| I9 | A terminated lock owner does not leave the semaphore permanently locked | `SEM_UNDO`, `src/system.c:61-77` | System V semantic oracle; runtime required |
| I10 | After all target processes terminate gracefully, target-created SHM, semaphore, and queue no longer exist | `src/game_algorithm.c:343-358` | INFERENCE from lifecycle intent; runtime required |
| I11 | One process receiving SIGINT drives peers toward END_GAME and bounded termination | `src/main.c:24-29,58-63`, `src/game_algorithm.c:319-323` | INFERENCE; runtime required |
| I12 | No supported scenario exceeds its explicit deadline; timeout is CRASH/deadlock evidence, not formatting noise | PROFILE C requirement | Test oracle |

Admission count and board placement use separate critical sections. Therefore I2/I3 will be asserted only at named stable points (after admission returns or after a bounded quiescence condition), not during an arbitrary transient snapshot.

## Planned deterministic cases

Initial suite: 26 representative cases. Exact repetition counts and deadlines will be calibrated once a successful smoke run exists; no numeric result is preclaimed.

| ID | Area | Scenario | Primary oracle |
|---|---|---|---|
| B01 | Build | Clean production and helper build with recorded compiler output | exit 0; expected binaries; source hashes unchanged |
| B02 | CLI | No argument, extra argument, nondigit, negative, and out-of-range team | nonzero exit and stable error class; no IPC creation |
| B03 | CLI | Empty team string and `viewmode`-prefix variants | exact dispatch/rejection behavior; compare to narrow documented interface |
| B04 | Creation | Start one player from no pre-existing target IPC | exactly one SHM, one 1-element semaphore set, one queue; recruitment state |
| B05 | Creation race | Release two first players simultaneously | one coherent IPC set, no hang, two unique board cells |
| B06 | Recruitment | One and two players on the same team | counts/cells agree; state remains recruitment |
| B07 | Capacity | Third player on a full team | rejection; counts, board, and existing processes unchanged |
| B08 | Barrier | Start 3 valid players, then the fourth | no early START; fourth causes START and all four leave wait within deadline |
| B09 | Late join | Attempt admission after START_GAME | rejection without state/count/board change |
| B10 | Placement | Repeat controlled four-player startup | every run has four unique in-bounds positions; exact coordinates not compared |
| B11 | Consistency | Snapshot stable state across gameplay ticks | I1-I3 hold at every accepted snapshot |
| B12 | Team queue | Inject team-A message while both teams run | only an A process may consume the A `mtype` message |
| B13 | Chase radius | Controlled target at squared distance ≤9 | next chase step follows exact dominant-axis rule |
| B14 | Out-of-radius | Controlled target at squared distance >9 with no existing target | one legal random adjacent attempt; no exact direction oracle |
| B15 | Greedy tie | Equal absolute x/y delta | y-axis step, matching `abs(dy) >= abs(dx)` |
| B16 | Boundary movement | Place target player on each edge/corner | no out-of-bounds state; position/cell remains coherent |
| B17 | Collision | Occupied intended next cell | no overlapping move; collision coordinate is sent; hold/fallback timing observed |
| B18 | Death | Same enemy team occupies both horizontal or vertical opposing cells | victim cell cleared, counts decrement once, victim exits |
| B19 | Non-death | Mixed teams, same-team neighbor, or only one opposing neighbor | no death transition from the surrounded predicate |
| B20 | Win/end | Reduce shared state to one live team | survivors terminate within deadline; graceful cleanup oracle applies |
| B21 | SIGINT | Send SIGINT to one waiting player and one active player in separate cases | END_GAME propagation, bounded peer exit, final IPC cleanup |
| B22 | Lock-owner exit | SIGKILL a helper/target while it owns the semaphore | SEM_UNDO restores progress; peers do not deadlock |
| B23 | Player SIGKILL | Kill one active player outside the lock | record stale cell/count behavior and whether remaining system progresses; classification respects undefined recovery scope |
| B24 | Full abnormal exit | SIGKILL all players | measure leftover IPC; next-run behavior; no automatic robustness claim |
| B25 | Repetition | Repeat complete graceful games sequentially | no cross-run stale state, leftover IPC, timeout, or crash |
| B26 | Viewer | Attach before/during/after a game and capture normalized output | 10 rows × 10 cells and counts match a near-time observer snapshot; bounded exit after SHM removal |

## Reference/oracle hierarchy

1. **Source-defined semantic oracle:** constants, state guards, message types, movement rules, death predicate, and cleanup condition cited above.
2. **Cross-field invariants:** count/board/state relationships at stable observation points.
3. **System V IPC semantics:** Linux behavior of `IPC_CREAT|IPC_EXCL`, `SEM_UNDO`, `IPC_RMID`, attach counts, nonblocking queue operations, and queue/semaphore metadata.
4. **Runtime liveness oracle:** explicit per-case deadline, process status, and absence/presence of IPC objects.
5. **README:** used only when code agrees or when evaluating the documented CLI. README claims alone are not treated as runtime proof.

No external gameplay implementation is used. GNU `nm` is not a relevant oracle for this repository; using it would follow the mismatched profile label rather than the target project.

## Normalization

### Normalized

- numeric PIDs and kernel-assigned SHM/semaphore/message-queue IDs;
- wall-clock timestamps and nondeterministic interleaving order of independent log lines;
- ANSI color and cursor-control escape sequences in viewer output;
- exact initial random coordinates when the case oracle is uniqueness, bounds, or count consistency;
- exact random fallback direction when the case oracle is membership in the legal one-step direction set;
- small scheduler/timing jitter inside an explicit deadline;
- polling sample count needed to reach a defined stable state.

### Not normalized

- exit code, terminating signal, timeout, deadlock, or process that remains alive past deadline;
- missing or extra process, board occupant, state transition, message, or IPC object;
- board coordinates for deterministic greedy/collision/death fixtures;
- incorrect global/team counts, illegal cell values, overlap, teleportation, or out-of-bounds effects;
- message `mtype`, `kind`, team, coordinates, loss, duplication, or wrong-team consumption;
- semaphore cardinality/value/progress and `SEM_UNDO` recovery outcome;
- stale SHM/semaphore/queue after a case whose graceful-cleanup oracle applies;
- invalid-input acceptance/rejection and diagnostic class;
- viewer row/cell/count content after escape-code removal.

Normalization will preserve raw stdout, stderr, observer snapshots, `ipcs` output, and process statuses under `portfolio_audit/raw/` only after approval to enter PHASE 4.

## Classification boundary

- Code inventory status does not become PASS without execution.
- Abnormal-exit logical repair is not promised by an implemented source path. Observed stale state will be reported as a measured robustness limitation unless it violates another explicit invariant; it will not be mislabeled as a correctness regression by assumption.
- A setup/harness failure is not a target CRASH.
- A target timeout/deadlock is CRASH even if partial output exists.

## Harness validation plan

After the baseline suite exists, 3-5 representative assertions will be mutation-checked using synthetic wrong snapshots/output: wrong player count, duplicate board occupant, wrong team message type, leftover IPC object, and deliberately exceeded deadline. This belongs to PHASE 7 and is not performed at CHECKPOINT A.
