#include "../include/logic.h"
#include "../include/system.h"
#include "../libft/libft.h"

int init_shm_and_pos_make(t_playerData *playerData){
  if (attach_shm(playerData) == -1) return (-1);
  if (lock_sem(playerData->sid) == -1) { perror("lock_sem"); return -1; }
  playerData->readwrite->game_state = RECRUITMENT;
  ft_memset(playerData->readwrite->board, 48, sizeof(playerData->readwrite->board));
  ft_memset(playerData->readwrite->team_nbs, 0, sizeof(playerData->readwrite->team_nbs));
  if (unlock_sem(playerData->sid) == -1) { perror("unlock_sem"); return -1; }
  t_pos pos[WIDTH * HEIGHT];
  make_rand_position(pos);
  int max_cnt = MAXTEAMNB * MAXTEAM;
  t_myMsgbuf msgbuf;
  msgbuf.mytype = BROADCAST;
  for (int i = 0; i < max_cnt; i++){
    msgbuf.x = pos[i].x;
    msgbuf.y = pos[i].y;
    msgbuf.kind = INIT_PLAYER;
    msgbuf.team_no = playerData->team_no;
    if (send_msg(playerData->qid, &msgbuf) == -1) return (-1);
  }
	return (0);
}

int update_player_data(t_playerData *playerData){
  if (playerData->readonly == NULL && playerData->readwrite == NULL){
    if (attach_shm(playerData) == -1) return (-1);
  }
  if (check_player_nbs(playerData) == -1) return (2);
  t_myMsgbuf msgbuf;
  if (recv_msg(playerData->qid, &msgbuf, BROADCAST) == -1) return (-1);
  if (msgbuf.kind == INIT_PLAYER){
    playerData->x = msgbuf.x;
    playerData->y = msgbuf.y;
    if (update_board(playerData, (t_pos){.x = playerData->x, .y = playerData->y}, playerData->team_no) == -1) return (-1);
  }
  else {
    send_msg(playerData->qid, &msgbuf);
    return (-1);
  }
  return (0);
}

int clear_not_reject_player(t_playerData *playerData){
  if (playerData->readonly != NULL ){
    if (detach_board(playerData->readonly) == -1) return (-1);
  }
  if (playerData->readwrite != NULL ){
    if (detach_board(playerData->readwrite) == -1) return (-1);
  }
  return (0);
}
