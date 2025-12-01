#include "../include/logic.h"
#include "../include/system.h"
#include <string.h>

void	init_playerdatata(t_playerData *data){
	data->mid = -1;
	data->sid = -1;
	data->qid = -1;
	data->t_key = -1;
	data->team_mb_cnt = 0;
	data->team_no = 0;
	data->x = 0;
	data->y = 0;
	data->readonly = NULL;
	data->readwrite = NULL;
}

int		check_player_nbs(t_playerData *player){
	int out_flag = 0;
	if (lock_sem(player->sid) == -1) { perror("lock_sem"); return 1; }
	if (player->readwrite->team_nbs[player->team_no] < MAXTEAMNB){
		player->readwrite->team_nbs[player->team_no] += 1;
		player->readwrite->player_nbs++;
	}
	else{
		out_flag = -1;
	}
	if (unlock_sem(player->sid) == -1) { perror("unlock_sem"); return 1; }
	return (out_flag);
}
