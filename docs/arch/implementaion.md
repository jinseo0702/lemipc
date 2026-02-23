IMPLEMENTATION DIAGRAM

[PROCESSINGFLOW]
1. '/home/jinseo/jinseo/42_gs/lemipc/docs/flowchart/swarm_intelligence_algorithm.xml' 의 내용을 준수해야한다.
[MODULERESPONSIBILITY]
기본 조건
각 player 는 메아리가 조건에 맞지 없거나/조건 미달이라면 랜덤으로 움직인다.
각 player 들은 장님이고, 적을 부딪쳐야지 누군지 파악 할 수 있다.
player는 팀원을 구분 할 수 없다.
player는 특정 팀원을 알 수 없다.
player 조건에 맞으면 반지름 3으로 메아리친다.

0. 적이 있다면 메세지를 보내고 가만히 있기를 선택한다.
1. player 는 msq를 확인한다.
	- msq 없으면 random 으로 움직인다.
2. msq 를 검사를 하고 메세지 보낸 팀원과의 자신과의 반지름 거리를 계산한다.
->  | x_t - x |^2 + | y_t - y |^2  <= 9
	- true 라면 적의 위치로 탐욕 알고리즘을 사용한다. 
		- 적위치 를 향해서  |x_e - x |  비교 | y_e - y| 
		   적 3, 17 나 5 , 12 , 결과 값 2 , 5 이떄 2 가 더 큰 숫자를 찾는다 이 경우 y  방향 움직이는데  절 대값을 뺀 y_e - y > 0 크다면 아래로 한칸(+) 이동 작다면 위로 한칸이동 (-) 한다.
	- false 라면 기존 적을추적 하거나 랜덤으로 움직인다.
3. 적 위치에 도달 했는데 적이 존재 하지 않는다면 죽은걸로 판단하고 랜덤 모드로 바뀐다.
4. 적을 추적 하는 중 옆에 적이 있다면 메세지를 보내고 그 가만히 있기를 선택 한다.
5. 다시 1번을 선택한다.

[DEFINITIONOFDONE]
1. '/home/jinseo/jinseo/42_gs/lemipc/docs/flowchart/swarm_intelligence_algorithm.xml' 의 내용을 준수해야한다.
2. 아래는 DoD입니다.

DoD-ELF (필수3)
함수가 flow chart 의 논리에 맞게 생성되었는지 확인
함수의 컴파일에 문제가 없는지 확인
함수가 flow chart에 의도한대로 동작하는지 확인