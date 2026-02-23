#include "../include/logic.h"
#include "../include/system.h"
#include "../printf/libftprintf.h"
#include "../include/game_algorithm.h"
#include "../libft/libft.h"

volatile sig_atomic_t g_got_sigint  = 0;

static void handler(int sig){
    if (sig == SIGINT)
        g_got_sigint = 1;
}

static int setup_signals(void){
    struct sigaction sa;
    ft_memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa, NULL) == -1) return -1;
    return 0;
}

void sig_is_come(t_playerData *player){
	if (g_got_sigint == 1){
		change_game_state_end(player);
		terminate_player(player);
		exit(0);
	}
}

int main(int argc, char *argv[]){
  int team_no = 0;
  if (argc == 2 && ft_strncmp(argv[1], "viewmode", 8) == 0) {
    while (1) {
      if (view_board() == 2) break;
      usleep(16667);
    }
    return (0);
  }
  else if (check_argument(argc, (const char **)argv, &team_no) == 1){
    return (1);
  }
  t_playerData playerData;
  init_playerdatata(&playerData, team_no);
  int flag = first_step(&playerData);
  if (check_first_step_flag(flag, team_no) == 1) {
    return (1);
  }

  if (setup_signals() == -1)
  {
	  perror("sigaction");
	  return 1;
  }

  t_myMsgbuf msgbuf;
  while (1) {
	sig_is_come(&playerData);
	if (playerData.readonly->game_state == END_GAME){
		terminate_player(&playerData);
		exit(0);
	}
    int error = recv_msg(playerData.qid, &msgbuf, START_GAME);
    if (error == ENOMSG) continue;
    if (msgbuf.kind == START_GAME) break;
    else if (error == -1) {
      ft_fprintf(2, "Fatal Error\n");
      return (1);
    }
  }
  sleep(2);
  run_game_loop(&playerData);
  return (0);
}
