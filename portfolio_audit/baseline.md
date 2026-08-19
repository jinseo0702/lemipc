# Baseline Environment

측정 시각 기준일: 2026-08-19 (Asia/Seoul)

## Repository

| Field | Value | Evidence |
|---|---|---|
| Commit | `108c4fc68c90e41723fe2ce35d6cb4b443524ed2` | VERIFIED FROM CODE (`git rev-parse HEAD`) |
| Commit date | `2026-03-10T23:32:46+09:00` | VERIFIED FROM CODE (`git log -1`) |
| Branch | `main` tracking `origin/main` | VERIFIED FROM CODE (`git status --short --branch`) |
| Dirty before audit | No tracked or untracked changes | VERIFIED FROM CODE (`git status --short`) |
| Dirty after CHECKPOINT A | Only new `portfolio_audit/` files | VERIFIED FROM CODE; final integrity check required |

## Host and toolchain

| Field | Value | Evidence |
|---|---|---|
| OS | Ubuntu 26.04 LTS (Resolute Raccoon) | VERIFIED FROM CODE (`/etc/os-release`) |
| Kernel | `7.0.0-29-generic` | VERIFIED FROM CODE (`uname -r`) |
| Architecture | `x86_64` | VERIFIED FROM CODE (`uname -m`) |
| Compiler | GCC/`cc` 15.2.0 (`Ubuntu 15.2.0-16ubuntu1`) | VERIFIED FROM CODE (`cc --version`) |
| libc | glibc 2.43 | VERIFIED FROM CODE (`getconf GNU_LIBC_VERSION`) |
| Make | GNU Make 4.4.1 | VERIFIED FROM CODE (`make --version`) |
| Python | Python 3.14.4 | VERIFIED FROM CODE (`python3 --version`) |
| IPC inspection tool | `ipcs` from util-linux 2.41.3 | VERIFIED FROM CODE (`ipcs --version`) |

## Build and run interface

| Purpose | Command | Derivation |
|---|---|---|
| Production build | `make` | VERIFIED FROM CODE (`Makefile:34-52`, `README.md:37-49`) |
| Existing helper build | `make test` | VERIFIED FROM CODE (`Makefile:41-55`) |
| Player | `./lemipc <team_no>` where code range is 0..1 | VERIFIED FROM CODE (`src/main.c:32-49`, `src/utils.c:105-121`) |
| Viewer | `./lemipc viewmode` | VERIFIED FROM CODE (`src/main.c:34-40`) |

The production target compiles six C translation units with `gcc -g -Wall -Wextra -Werror` and links local `libft/libft.a` and `printf/libftprintf.a` (`Makefile:1-38`).

**CHECKPOINT A execution status:** build and runtime commands have not been executed. Build success, runtime behavior, PASS/PARTIAL/FAIL/CRASH counts, timing, leak behavior, and deadlock freedom are `UNKNOWN`.

## Relevant reference/oracle

There is no external reference implementation selected for gameplay semantics.

1. Primary oracle: invariants derived from the code's constants, state transitions, critical sections, and resource lifecycle.
2. Secondary oracle: Linux/System V IPC API semantics for `shm*`, `sem*`, and `msg*`; installed observation tool is util-linux `ipcs` 2.41.3.
3. Operational oracle: bounded process completion, exit status/signal, IPC object existence, queue metadata, semaphore state, and shared-memory snapshots captured by an audit helper.

This selection is `INFERENCE` from the repository-specific PROFILE C instructions. Exact expected gameplay beyond the source-defined rules remains `UNKNOWN` until a user rationale or specification is supplied.

## Source integrity baseline

`source_sha256.txt` records the production Makefile, six production C files, and four public headers. Aggregate hashes were also calculated over sorted per-file SHA-256 output:

| Scope | Aggregate SHA-256 | Evidence |
|---|---|---|
| `src/*.c` and `include/*.h`, including repository test C files | `88df156d10d6b32730c2d6acb61803aada6d3b09534a934eeca73a19f5a6d9aa` | VERIFIED FROM CODE |
| all C/header/Makefile files under `libft/` and `printf/` | `d9326380d7acc6d938bd8a663d5189da85de6aa5090402d98d8fa36050c7ceb5` | VERIFIED FROM CODE |

Aggregate method: sort file paths bytewise, compute each file's SHA-256, then SHA-256 the resulting manifest stream. These hashes establish identity only; they do not imply correctness.
