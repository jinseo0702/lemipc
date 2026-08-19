# Harness Validation

Method: predicate-level mutation validation. Each checker must accept a valid synthetic observation and reject one deliberately corrupted observation.

| ID | Mutation | Valid accepted? | Wrong rejected? | Result |
|---|---|---:|---:|---|
| HV01 | wrong global count | True | True | PASS |
| HV02 | missing board occupant | True | True | PASS |
| HV03 | leftover shared memory | True | True | PASS |
| HV04 | semaphore remains locked | True | True | PASS |
| HV05 | deadline exceeded | True | True | PASS |

Result: 5 PASS, 0 FAIL.

This validates representative count, board, cleanup, semaphore, and deadline predicates. It does not prove that every test setup is correct; setup failures remain separately classified.
