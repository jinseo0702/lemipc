#include "../include/logic.h"
#include "../include/system.h"
#include "../printf/libftprintf.h"
#include "../include/game_algorithm.h"

int main(int argc, char *argv[]){
  int team_no = 0;
  if (check_argument(argc, (const char **)argv, &team_no) == 1){
    return (1);
  }
  t_playerData playerData;
  init_playerdatata(&playerData, team_no);
  int flag = first_step(&playerData);
  if (check_first_step_flag(flag, team_no) == 1) {
    return (1);
  }
  t_myMsgbuf msgbuf;
  while (1) {
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
  terminate_player(&playerData);
  return (0);
}
