#include "../include/logic.h"
#include "../include/system.h"
#include <string.h>

int first_step(t_playerData *playerData){
	int first_flag = 0;
	if ((first_flag = init_ipcs_ids(playerData)) == -1) return (-1);
	if (first_flag == 2){
		if (init_shm_and_pos_make(playerData) == -1) return (-1);
	}
	if (update_player_data(playerData) == 2) {
		clear_not_reject_player(playerData);
		return (1); //정원가득참
	}
	int total_player_nbs = return_total_player_nbs(playerData);
	if (total_player_nbs == -1) return (-1);
	int max_player_nbs = MAXTEAMNB * MAXTEAM;
	if (total_player_nbs == max_player_nbs) {
		t_myMsgbuf msgbuf;
		msgbuf.mytype = START_GAME;
		msgbuf.x = 0;
		msgbuf.y = 0;
		msgbuf.kind = START_GAME;
		msgbuf.team_no = NONE;
		msgbuf.msg_order = DO_NOTHING;
		for (int i = 0; i < max_player_nbs; i++) {
			if (send_msg(playerData->qid, &msgbuf) == -1) return (-1);
		}
	}
	return (0);
}

void	init_playerdatata(t_playerData *data, int team_no){
	data->mid = -1;
	data->sid = -1;
	data->qid = -1;
	data->t_key = -1;
	data->team_mb_cnt = 0;
	data->team_no = team_no + 'A';
	data->x = 0;
	data->y = 0;
	data->readonly = NULL;
	data->readwrite = NULL;
}

int		check_player_nbs(t_playerData *player){
	int out_flag = 0;
	if (lock_sem(player->sid) == -1) { perror("lock_sem"); return 1; }
	if (player->readwrite->team_nbs[player->team_no - 'A'] < MAXTEAMNB){
		player->readwrite->team_nbs[player->team_no - 'A'] += 1;
		player->readwrite->player_nbs++;
		player->team_mb_cnt = player->readwrite->team_nbs[player->team_no - 'A'];
	}
	else{
		out_flag = -1;
	}
	if (unlock_sem(player->sid) == -1) { perror("unlock_sem"); return 1; }
	return (out_flag);
}

void	make_rand_position(t_pos *pos){
	srand(time(NULL));
	int max_idx = WIDTH * HEIGHT;
	t_pos temp_pos;
	int re_pos = 0;
	for (int i = 0; i < WIDTH; i++)
		for (int j = 0; j < HEIGHT; j++)
			pos[re_pos++] = (t_pos){i, j};
	for (int i = max_idx - 1; i > 0; i--){
		int rand_idx = rand() % (i + 1);
		temp_pos = pos[i];
		pos[i] = pos[rand_idx];
		pos[rand_idx] = temp_pos;
	}
}

int attach_shm(t_playerData *playerData){
	playerData->readonly = readonly_board(playerData->mid);
	if (playerData->readonly == NULL){
		return (-1);
	}
	playerData->readwrite = readwrite_board(playerData->mid);
	if (playerData->readwrite == NULL){
		return (-1);
	}
	return (0);
}

int update_board(t_playerData *playerData, t_pos pos, int team_no){
  if (lock_sem(playerData->sid) == -1) { perror("lock_sem"); return (-1); }
  playerData->readwrite->board[pos.x][pos.y] = team_no;
  if (unlock_sem(playerData->sid) == -1) { perror("unlock_sem"); return (-1); }
  return (0);
}

int return_total_player_nbs(t_playerData *playerData){
	int total_player_nbs = 0;
	if (lock_sem(playerData->sid) == -1) { perror("lock_sem"); return (-1); }
	total_player_nbs = playerData->readwrite->player_nbs;
	if (unlock_sem(playerData->sid) == -1) { perror("unlock_sem"); return (-1); }
	return (total_player_nbs);
}

int return_total_player_team_nbs(t_playerData *playerData, int *total_player_nbs){
	if (lock_sem(playerData->sid) == -1) { perror("lock_sem"); return (-1); }
	for (int i = 0; i < MAXTEAM; i++) {
		total_player_nbs[i] = playerData->readwrite->team_nbs[i];
	}
	if (unlock_sem(playerData->sid) == -1) { perror("unlock_sem"); return (-1); }
	return (0);
}

// int middle_step(t_playerData *playerData){

// }

int recv_msg_check(t_playerData *playerData, t_myMsgbuf *msgbuf){
	int team_no = playerData->team_no;
	int error = recv_msg(playerData->qid, msgbuf, team_no);
	if (error == ENOMSG) return (0);
	if (error == -1) return (-1);
	return (1);
}