#include "../include/logic.h"
#include "../include/system.h"
#include <stdio.h>

#define RESET   "\033[0m"
#define GRAY    "\033[90m"

// 글자색 (Foreground)
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// 배경색 (Background)
#define BG_RED     "\033[41m"
#define BG_GREEN   "\033[42m"
#define BG_YELLOW  "\033[43m"
#define BG_BLUE    "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN    "\033[46m"
#define BG_WHITE   "\033[47m"

const char *get_team_color(int team_no) {
    switch(team_no) {
        case 0: return RED;         // 빨강 글씨
        case 1: return GREEN;       // 초록 글씨
        case 2: return YELLOW;      // 노랑 글씨
        case 3: return BLUE;        // 파랑 글씨
        case 4: return MAGENTA;     // 자주 글씨
        case 5: return CYAN;        // 청록 글씨
        case 6: return BG_WHITE RED;    // 흰 배경 + 빨강 글씨
        case 7: return BG_WHITE BLUE;   // 흰 배경 + 파랑 글씨
        case 8: return BG_WHITE MAGENTA;// 흰 배경 + 자주 글씨
        
        default: return RESET;
    }
}

void view_board(void){
  int shmid;
  key_t key = ftok(PATHNAME, PROJID);
  if (key == -1){
    perror("ftok");
    return;
  }
  shmid = shmget(key, sizeof(t_shm), 0666);
  if (shmid == -1){
    perror("shmget");
    return;
  }
  t_shm *readonly = readonly_board(shmid);
  if (readonly == NULL){
    perror("readonly_board");
    return;
  }
  printf("\n=== Board State (Players: %d) ===\n", readonly->player_nbs);
  for (int i = 0; i < HEIGHT; i++){
    for (int j = 0; j < WIDTH; j++){  
      printf("%s%c%s ", get_team_color(readonly->board[i][j] - 'A'), readonly->board[i][j], RESET);
    }
    printf("\n");
  }
  printf("team info:\n");
  printf("--------------------------------\n");
  for (int i = 0; i < MAXTEAM; i++){
    printf("%s%d -> %c%s team: %d players\n", get_team_color(i), i, i + 'A', RESET, readonly->team_nbs[i]);
  }
  printf("\n");
  detach_board(readonly);
}