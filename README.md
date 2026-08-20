# lemipc

`lemipc`는 여러 독립 프로세스가 10×10 보드와 게임 상태를 공유하는 System V IPC 시뮬레이션입니다. 공유 메모리, 세마포어, 메시지 큐를 함께 사용해 두 팀의 플레이어가 하나의 게임을 진행합니다.

## 만든 이유

한 프로세스 안의 스레드가 아니라 서로 다른 프로세스가 상태를 공유하고 동기화하는 방법을 직접 이해하기 위해 만들었습니다. 특히 공유 상태의 일관성, 시작 barrier, 프로세스 종료 뒤 IPC 자원 회수처럼 멀티프로세스 환경에서만 드러나는 lifecycle 문제를 설계 대상으로 삼았습니다.

## 핵심 기능

- 공유 메모리에 보드, 전체 인원, 팀별 인원, 게임 상태를 저장합니다.
- 전역 세마포어 하나로 참가·이동·사망 같은 공유 상태 변경을 직렬화하며 `SEM_UNDO`를 사용합니다.
- typed message queue 하나를 초기 위치, 게임 시작, 팀 메시지 채널로 나누어 사용합니다.
- 두 팀에 각 2명씩, 총 4명이 모이면 `START_GAME` 상태로 전환합니다.
- 반경 3 이내 팀 메시지는 greedy 이동의 목표로 삼고, 목표가 없으면 인접 8방향 중 하나로 이동합니다.
- `viewmode`는 게임 중 공유 보드와 팀별 인원을 표시합니다.

## 동작 구조

```text
player 0/1
  -> 첫 프로세스가 SHM 생성으로 초기화 담당 결정
  -> SHM + semaphore + message queue 생성/접속
  -> 팀 정원 확인, 초기 위치 수신, 보드 배치
  -> 4명 참가 시 START_GAME 메시지 4개 전송
  -> 팀 메시지 수신 -> semaphore lock -> 종료/사망/이동 판정
  -> 두 SHM mapping 분리 -> 마지막 attachment이면 IPC 제거 시도

viewmode -> SHM attach -> 보드 snapshot 출력 -> detach -> 반복
```

첫 프로세스는 `IPC_CREAT | IPC_EXCL`로 선출됩니다. 각 플레이어는 같은 SHM을 read-only와 read-write로 각각 attach하고, 게임 루프의 핵심 판정과 변경을 하나의 critical section에서 수행합니다.

## 설계하면서 고민한 점

- 전역 세마포어와 typed queue를 선택해 lock ordering을 늘리지 않으면서 deadlock과 data race 가능성을 줄이는 데 집중했습니다. 이 단순화의 성능 효과는 측정하지 않았습니다.
- 공유 메모리는 프로세스들이 같은 내용을 직접 확인하는 통신 수단으로 선택했습니다.
- 마지막 플레이어 종료 뒤 자원을 자동으로 회수하고 Viewer도 자동 종료하는 lifecycle을 의도했습니다.
- `DEATH` 메시지는 팀원이 게임 종료 여부를 확인하도록 만든 신호입니다. 현재 수신 경로의 실제 동작에는 아래 한계가 있습니다.
- 원래 지원한 cooperative 종료 경로는 `SIGINT`입니다.

## Build

Linux의 System V IPC를 사용하는 C 프로젝트입니다.

```bash
make
```

기본 target은 실행 파일 `lemipc`를 만듭니다. 보조 테스트 실행 파일은 다음과 같이 빌드합니다.

```bash
make test
```

## Run

Viewer를 한 터미널에서 실행합니다.

```bash
./lemipc viewmode
```

다른 네 터미널에서 팀 0과 팀 1의 플레이어를 각각 두 명씩 실행합니다.

```bash
./lemipc 0
./lemipc 0
./lemipc 1
./lemipc 1
```

허용되는 팀 번호는 `0`, `1`이며 네 명이 모두 참가해야 게임이 시작됩니다. 플레이어에서 `Ctrl+C`를 누르면 공유 상태를 `END_GAME`으로 바꾸는 cooperative 종료 경로가 실행됩니다.

비정상 종료 뒤 이 프로젝트의 고정 IPC key에 자원이 남았다면 보조 도구로 정리할 수 있습니다.

```bash
./testSystem test_sem_clear_ipcs
```

## 검증 결과

| 범위 | 결과 |
|---|---|
| 비교 기준 | 소스 기반 불변식, Linux System V IPC 의미, bounded process/IPC lifecycle |
| 실행 case | 23 |
| 분류 | PASS 16 · PARTIAL 4 · FAIL 2 · CRASH 1 |
| 대표 관찰 | 동시 IPC 생성, 4-player start barrier, `SIGINT` 전파와 cleanup, `SEM_UNDO` 및 semaphore stress를 확인 |

전체 결과: [`portfolio_audit/test_results.md`](portfolio_audit/test_results.md)

## 확인된 한계

- B13은 8회의 Viewer+player graceful 종료 중 1회에서 Viewer와 세 종류의 IPC가 남은 lifecycle/liveness 문제입니다. `CRASH` 분류는 segmentation fault가 아니라 Viewer가 끝나지 않아 deadline을 넘긴 결과입니다.
- `DEATH` 메시지는 별도 분기 없이 일반 좌표처럼 추적 목표가 된 뒤 공통 게임 종료 검사를 거칩니다. 메시지의 원래 목적과 수신 동작이 완전히 일치하지 않습니다.
- `SIGTERM`/`SIGKILL` 뒤에는 ghost player와 IPC가 남을 수 있습니다. 이는 측정된 robustness limitation이며, 원래 지원 범위였던 `SIGINT` 종료와 구분해야 합니다.

## 상세 문서

- [Audit 개요](portfolio_audit/README.md)
- [Architecture](portfolio_audit/architecture.md)
- [CHECKPOINT B](portfolio_audit/checkpoint_b.md)
- [Failure analysis](portfolio_audit/failures.md)
- [Design rationale](portfolio_audit/design_rationale.md)
- [초기 설계 문서](docs/arch/architecture.md)
