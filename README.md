# lemipc

`lemipc`는 10x10 공유 보드 위에서 동작하는 System V IPC 기반 멀티프로세스 시뮬레이션입니다. 메시지 큐로 팀 단위 정보를 공유하고, 세마포어로 공유 상태 접근을 동기화합니다.

## 개요

- 보드 크기: `10 x 10`
- 팀 수: `2`개 (`0 -> A`, `1 -> B`)
- 팀당 최대 인원: `2`
- 사용 IPC:
  - 공유 메모리: 보드와 게임 상태 저장
  - 세마포어: 공유 상태 접근 동기화
  - 메시지 큐: 팀 통신 및 게임 시작 신호 전달
- 게임 시작 조건: 전체 `4`명의 플레이어가 모두 입장해야 시작

## 동작 구조

구현은 [`docs/arch`](docs/arch)와 [`docs/flowchart`](docs/flowchart)에 있는 설계 문서와 플로우차트를 기준으로 구성되어 있습니다.

1. 첫 번째 플레이어가 IPC 자원을 생성하고 공유 메모리를 `RECRUITMENT` 상태로 초기화합니다.
2. 시작 위치는 랜덤으로 생성되며 메시지 큐를 통해 배분됩니다.
3. 각 플레이어는 팀에 참가하고 공유 메모리에 붙은 뒤 자신의 시작 좌표를 차지합니다.
4. 모든 플레이어가 입장하면 `START_GAME` 메시지가 브로드캐스트됩니다.
5. 메인 루프에서 각 플레이어는 다음 순서로 동작합니다.
   - 팀 메시지를 확인합니다.
   - 세마포어를 잠급니다.
   - 현재 보드 상태를 읽습니다.
   - 한 팀만 남았으면 종료 절차로 들어갑니다.
   - 적에게 둘러싸인 상태면 사망 처리합니다.
   - 그 외에는 swarm 알고리즘을 수행합니다.
     - 반경 `3` 이내의 팀 메시지가 있으면 그 좌표를 추적합니다.
     - 추적 중이면 greedy 방식으로 목표 좌표에 접근합니다.
     - 유효한 목표가 없으면 랜덤 이동으로 전환합니다.
     - 이동 중 충돌이 발생하면 목표 좌표를 팀에 알리고 잠시 정지합니다.
6. 종료 시 각 프로세스는 공유 메모리에서 분리되며, 마지막 프로세스가 IPC 자원을 해제합니다.

## 빌드

```bash
make
```

자주 쓰는 타깃:

- `make`: `lemipc` 빌드
- `make test`: `testSystem` 빌드
- `make clean`
- `make fclean`
- `make re`

## 실행

플레이어 실행:

```bash
./lemipc <team_no>
```

- 가능한 `team_no`: `0` 또는 `1`
- 그 외 값은 모두 거부됩니다.

뷰어 모드:

```bash
./lemipc viewmode
```

권장 실행 순서:

터미널 1:

```bash
./lemipc viewmode
```

터미널 2-5:

```bash
./lemipc 0
./lemipc 0
./lemipc 1
./lemipc 1
```

참고:

- 전체 `4`명이 입장해야 게임이 시작됩니다.
- 플레이어 프로세스에서 `Ctrl+C`를 누르면 공유 게임 상태가 `END_GAME`으로 변경됩니다.
- 공유 메모리가 제거되면 뷰어는 자동으로 종료됩니다.

## 테스트 및 보조 도구

먼저 보조 바이너리를 빌드합니다.

```bash
make
make test
```

파이썬 테스트 실행:

```bash
python3 testsystem.py
```

멀티프로세스 시뮬레이션 실행:

```bash
python3 testlemipc2.py
```

실행이 비정상 종료되어 IPC가 남았을 때 정리:

```bash
./testSystem test_sem_clear_ipcs
```

## 문서 위치

설계 문서:

- [`docs/arch/architecture.md`](docs/arch/architecture.md)
- [`docs/arch/implementaion.md`](docs/arch/implementaion.md)
- [`docs/arch/how_to_modify.txt`](docs/arch/how_to_modify.txt)

플로우차트:

- [`docs/flowchart/main.xml`](docs/flowchart/main.xml)
- [`docs/flowchart/init.xml`](docs/flowchart/init.xml)
- [`docs/flowchart/mainLoop.xml`](docs/flowchart/mainLoop.xml)
- [`docs/flowchart/swarm_intelligence_algorithm.xml`](docs/flowchart/swarm_intelligence_algorithm.xml)
- [`docs/flowchart/Termination.xml`](docs/flowchart/Termination.xml)

## 프로젝트 구조

```text
include/   헤더와 공용 데이터 구조체 정의
src/       런타임 로직, IPC 처리, 게임 루프 구현
libft/     로컬 libft 의존성
printf/    로컬 printf 의존성
docs/      설계 문서와 플로우차트
```
