# lemipc Portfolio Audit — CHECKPOINT B

이 디렉터리는 commit `108c4fc68c90e41723fe2ce35d6cb4b443524ed2`의 정적 audit와 source 수정 전 runtime baseline 결과다.

## 현재 단계

- 완료: PHASE 0 — Baseline Environment
- 완료: PHASE 1 — Repository Architecture Audit
- 완료: PHASE 2 — Feature Inventory
- 완료: PHASE 3 — Scope / Size Measurement
- 완료: PHASE 4 — Deterministic Test Suite
- 완료: PHASE 5 — Oracle / Reference Comparison
- 완료: PHASE 6 — Classification
- 완료: PHASE 7 — Test Harness Validation
- 완료: PHASE 8 — Failure Analysis
- 완료: PHASE 9 — Design Rationale
- 완료: PHASE 10 — Improvement Candidate Selection
- 도달: source-change approval boundary after CHECKPOINT B
- 미수행: improvement 구현, regression/Before-After, source 수정

기존 `src/`, `include/`, `libft/`, `printf/` 및 기존 테스트 source는 수정하지 않았다. Audit harness, probe, raw evidence와 보고서만 `portfolio_audit/`에 추가했다.

## Profile 해석

사용자 요청에는 `PROFILE B`라고 적혀 있지만, 제공된 audit 명세에서 PROFILE B는 `gnu_nm_project`, `lemipc`는 PROFILE C로 정의되어 있다. 대상 저장소를 `lemipc`로 명시한 지시를 우선하여 이 audit은 문서상 **PROFILE C — multi-process IPC/concurrency/resource lifecycle** 기준을 사용했다.

이 해석은 `INFERENCE`이며, 나머지 프로젝트 사실과 분리한다.

## 문서

- `baseline.md`: commit, 환경, toolchain, 실행 경계, source integrity
- `architecture.md`: 실제 코드에서 도출한 architecture, functional scope, design characteristics
- `feature_inventory.md`: 기능별 코드 구현 상태와 source evidence
- `test_plan.md`: invariant 기반 baseline test plan, oracle, normalization
- `test_results.md` / `test_results.json`: 23-case baseline 결과
- `failures.md`: 5개 주요 failure/limitation의 root-cause 분석
- `harness_validation.md` / `.json`: 5개 synthetic mutation 검출 결과
- `checkpoint_b.md`: CHECKPOINT B 요약, 개선 architecture 후보, 사용자 질문
- `design_rationale.md`: 사용자 답변과 검증 사실/추론/미확인 사항의 분리 기록
- `improvement.md`: 우선순위와 권장 solution architecture
- `source_sha256.txt`: production source와 header의 CHECKPOINT A SHA-256

## Evidence label

- `VERIFIED FROM CODE`: 현재 source 또는 repository metadata로 확인
- `VERIFIED FROM RUNTIME TEST`: 실제 실행으로 확인 — CHECKPOINT A에는 없음
- `USER-PROVIDED RATIONALE`: 사용자가 설명한 설계 의도 — CHECKPOINT A에는 없음
- `INFERENCE`: 코드 사실에서 도출했지만 아직 runtime으로 검증하지 않은 해석
- `UNKNOWN`: 현재 evidence로 판단 불가

## Baseline headline

- PASS: 16
- PARTIAL: 4
- FAIL: 2
- CRASH: 1
- HARNESS_ERROR: 0

가장 중요한 runtime evidence는 viewer 동시 실행 시 8회의 graceful shutdown 중 1회에서 마지막 player가 exit 0으로 끝났지만 SHM/semaphore/message queue와 viewer가 남은 lifecycle race다.

## Reproduction

Repository root에서:

```bash
make
make test
cc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -Iinclude \
  portfolio_audit/tools/ipc_probe.c -o portfolio_audit/bin/ipc_probe
cc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -Iinclude \
  portfolio_audit/tools/algorithm_probe.c -o portfolio_audit/bin/algorithm_probe
python3 portfolio_audit/tests/run_baseline.py
python3 portfolio_audit/tests/validate_harness.py
```

Baseline 종료 후 생성된 repository-root build artifact는 `make fclean`으로 제거했다. Audit helper binary와 raw evidence는 `portfolio_audit/`에 보존했다.

## 승인 경계

`improvement.md`의 F01/Architecture A를 명시적으로 승인받기 전에는 source 변경과 PHASE 11 improvement cycle을 수행하지 않는다.
