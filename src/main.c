#include "../include/logic.h"
#include "../include/system.h"
#include "../printf/libftprintf.h"

int main(int argc, char **argv){
	if (argc != 2){
		ft_fprintf(2, "Usage: %s <team_no>\n", argv[0]);
		return (1);
	}
	int team_no = atoi(argv[1]);
  if (team_no < 0 || team_no > MAXTEAM - 1) {
    ft_fprintf(2, "Invalid team number: %d\n", team_no);
    return (1);
  }
  t_playerData playerData;
  init_playerdatata(&playerData, team_no);
  int flag = first_step(&playerData);
  if (flag == 1) {
    ft_fprintf(2, "Team %d is full\n", team_no);
    return (1);
  }
  else if (flag == -1) {
    ft_fprintf(2, "Fatal Error\n");
    return (1);
  }
  t_myMsgbuf msgbuf;
  while (1) {
    int error = recv_msg(playerData.qid, &msgbuf, START_GAME);
    if (error == ENOMSG) continue;
    if (msgbuf.kind == START_GAME) {
      break;
    }
    else if (error == -1) {
      ft_fprintf(2, "Fatal Error\n");
      return (1);
    }
  }
  sleep(2); // 2초 대기
	return (0);
}
