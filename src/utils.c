#include "../include/logic.h"
#include "../include/system.h"
#include "../printf/libftprintf.h"
#include "../libft/libft.h"
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

int view_board(void){
  static int game_start_flag = 0;
  int shmid;
  key_t key = ftok(PATHNAME, PROJID);
  if (key == -1){
    perror("ftok");
    return (-1);
  }
  shmid = shmget(key, sizeof(t_shm), 0666);
  if (game_start_flag == 0 && shmid == -1) {
    return (0);
  }
  else if (game_start_flag == 0 && shmid != -1) {
    game_start_flag = 1;
    ft_printf("Game Start\n");
  }
  else if (shmid == -1 && game_start_flag == 1) {
    return (2);
  }
  t_shm *readonly = readonly_board(shmid);
  if (readonly == NULL){
    perror("readonly_board");
    return (-1);
  }
  ft_printf("\033[H"); // cursor home (전체 clear 안 함)
  ft_printf("=== Board State (Players: %d) ===\033[K\n", readonly->player_nbs);
  for (int i = 0; i < HEIGHT; i++){
    for (int j = 0; j < WIDTH; j++){  
		char c = readonly->board[i][j];
		const char *color = (c == '0') ? GRAY : get_team_color(c - 'A');
		ft_printf("%s%c%s ", color, c, RESET);
    }
    ft_printf("\033[K\n");
  }
  ft_printf("team info:\033[K\n");
  ft_printf("--------------------------------\033[K\n");
  for (int i = 0; i < MAXTEAM; i++){
    ft_printf("%s%d -> %c%s team: %d players\033[K\n", get_team_color(i), i, i + 'A', RESET, readonly->team_nbs[i]);
  }
  ft_printf("\033[J");
  fflush(stdout);
  detach_board(readonly);
  return (0);
}

void view_board_player(t_playerData *playerData){
	ft_printf("\n=== Board State (Players: %d) ===\n", playerData->readonly->player_nbs);
	for (int i = 0; i < HEIGHT; i++){
		for (int j = 0; j < WIDTH; j++){  
			ft_printf("%s%c%s ", get_team_color(playerData->readonly->board[i][j] - 'A'), playerData->readonly->board[i][j], RESET);
		}
		ft_printf("\n");
	}
	ft_printf("team info:\n");
	ft_printf("--------------------------------\n");
	for (int i = 0; i < MAXTEAM; i++){
		ft_printf("%s%d -> %c%s team: %d players\n", get_team_color(i), i, i + 'A', RESET, playerData->readonly->team_nbs[i]);
	}
	ft_printf("\n");
}

int check_argument(int argc, const char *argv[], int *team_no){
	if (argc != 2){
		ft_fprintf(2, "Usage: %s <team_no>\n", argv[0]);
		return (1);
	}
	for (int i = 0 ; argv[1][i]; i++) {
		if (ft_isdigit(argv[1][i]) == 0) {
			ft_fprintf(2, "Invalid team number: %s\n", argv[1]);
			return (1);
		}
	}
	*team_no = ft_atoi(argv[1]);
	if (*team_no < 0 || *team_no > MAXTEAM - 1) {
		ft_fprintf(2, "Invalid team number: %d\n", *team_no);
		return (1);
	}
	return (0);
}

int	check_first_step_flag(int flag, int team_no){
	if (flag == 1) {
		ft_fprintf(2, "Team %d is full\n", team_no);
		return (1);
	}
	else if (flag == -1) {
		ft_fprintf(2, "Fatal Error\n");
		return (1);
	}
	return (0);
}