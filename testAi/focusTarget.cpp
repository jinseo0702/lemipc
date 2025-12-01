#include <iostream>
#include <cstring>
#include <cstdlib> // rand, srand
#include <ctime>   // time
#include <unistd.h> // usleep

#define MAX_PLAYERS 4
#define BOARD_SIZE 10

// 플레이어 구조체
typedef struct s_player{
    int id;
    int x, y;
    int finished; // 목표에 도달했는지 여부 (1: 완료, 0: 진행중)
} player;

// 전역 변수
char board[BOARD_SIZE][BOARD_SIZE]; // 시각화를 위한 보드 (char로 변경하여 문자 출력 용이하게)
player players[MAX_PLAYERS];
int goal_x, goal_y; // 목표 지점
int players_finished_count = 0; // 완료한 플레이어 수

// --- 유틸리티 모듈 ---

// 랜덤 좌표 생성 (보드 범위 내)
void get_random_pos(int *x, int *y) {
    *x = rand() % BOARD_SIZE;
    *y = rand() % BOARD_SIZE;
}

// --- 초기화 모듈 ---

void init_game(void){
    srand(time(NULL)); // 랜덤 시드 초기화
    
    // 플레이어 초기화
    for(int i = 0; i < MAX_PLAYERS; i++) {
        players[i].id = i + 1;
        players[i].finished = 0;
        get_random_pos(&players[i].x, &players[i].y);
    }

    // 첫 Goal 설정
    get_random_pos(&goal_x, &goal_y);
    players_finished_count = 0;
}

// --- 디스플레이 모듈 ---

void display_board(void){
    // 보드 초기화 (배경은 '.')
    for(int y = 0; y < BOARD_SIZE; y++) {
        for(int x = 0; x < BOARD_SIZE; x++) {
            board[y][x] = '.';
        }
    }

    // Goal 표시 ('G')
    board[goal_y][goal_x] = 'G';

    // 플레이어 표시 (완료하지 않은 플레이어만 표시하거나, 숫자로 표시)
    for(int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].finished) {
            // 플레이어 번호 ('1' ~ '4')
            board[players[i].y][players[i].x] = players[i].id + '0';
        }
    }

    // 화면 출력
    system("clear"); // 터미널 청소 (윈도우는 cls, 리눅스/맥은 clear)
    
    std::cout << "=== AI Tracking Simulation ===" << std::endl;
    std::cout << "Goal Position: (" << goal_x << ", " << goal_y << ")" << std::endl;
    std::cout << "Finished Players: " << players_finished_count << " / " << MAX_PLAYERS << std::endl;
    std::cout << "------------------------------" << std::endl;

    for(int y = 0; y < BOARD_SIZE; y++) {
        for(int x = 0; x < BOARD_SIZE; x++) {
            std::cout << " " << board[y][x] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}

// --- AI 로직 모듈 ---

// 해당 위치에 다른 플레이어가 있는지 확인 (본인 제외)
int is_position_occupied(int x, int y, int my_id) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // 본인이거나 이미 완료한 플레이어는 충돌 대상 아님
        if (players[i].id == my_id || players[i].finished) continue;

        if (players[i].x == x && players[i].y == y) {
            return 1; // 누군가 있음
        }
    }
    return 0; // 비어있음
}

// 각 플레이어가 Goal을 향해 1칸 이동 (충돌 방지 포함)
void move_players(void){
    for(int i = 0; i < MAX_PLAYERS; i++) {
        // 이미 완료한 플레이어는 움직이지 않음
        if (players[i].finished) continue;

        // 1. X축 이동 시도
        int next_x = players[i].x;
        if (players[i].x < goal_x) next_x++;
        else if (players[i].x > goal_x) next_x--;

        // X축 방향으로 이동했을 때 충돌이 없으면 이동 확정
        if (!is_position_occupied(next_x, players[i].y, players[i].id)) {
            players[i].x = next_x;
        }
        // 충돌이 있다면 X축 이동 포기 (제자리 유지)

        // 2. Y축 이동 시도
        int next_y = players[i].y;
        if (players[i].y < goal_y) next_y++;
        else if (players[i].y > goal_y) next_y--;

        // Y축 방향으로 이동했을 때 충돌이 없으면 이동 확정
        // 주의: 위에서 X축 이동이 일어났다면, 변경된 X 좌표 기준으로 Y축 충돌 체크
        if (!is_position_occupied(players[i].x, next_y, players[i].id)) {
            players[i].y = next_y;
        }
        // 충돌이 있다면 Y축 이동 포기 (제자리 유지)
    }
}

// --- 게임 상태 업데이트 모듈 ---

// Goal 도달 여부 체크 및 업데이트
void check_goal(void){
    int goal_reached_this_tick = 0;

    for(int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].finished) continue;

        // 플레이어가 Goal 좌표에 도달했는지 확인
        if (players[i].x == goal_x && players[i].y == goal_y) {
            players[i].finished = 1; // 완료 처리
            players_finished_count++;
            goal_reached_this_tick = 1;
            
            // 로그 출력 (너무 빠르면 안 보일 수 있음)
            std::cout << "Player " << players[i].id << " reached the Goal!" << std::endl;
        }
    }

    // 누군가 Goal에 도달했고, 아직 게임이 안 끝났다면 새로운 Goal 설정
    if (goal_reached_this_tick && players_finished_count < MAX_PLAYERS) {
        get_random_pos(&goal_x, &goal_y);
    }
}

// --- 메인 함수 ---

int main(void){
    init_game();

    while (players_finished_count < MAX_PLAYERS) {
        display_board();
        move_players();
        check_goal();
        
        usleep(1000000); // 1초 대기 (애니메이션 속도 조절)
    }

    // 최종 결과 출력
    display_board();
    std::cout << "\nAll players have reached the goal! Game Over." << std::endl;

    return 0;
}