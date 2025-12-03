#ifndef LOGIC_H
#define LOGIC_H

#include "./data.h"
#include <errno.h>
#include <time.h>

//logic.c
void	init_playerdatata(t_playerData *data, int team_no);
int		check_player_nbs(t_playerData *player);
void	make_rand_position(t_pos *pos);
int   first_step(t_playerData *playerData);
int   attach_shm(t_playerData *playerData);
int   update_board(t_playerData *playerData, t_pos pos, int team_no);
int   return_total_player_nbs(t_playerData *playerData);

//main.c
//player.c
int   init_shm_and_pos_make(t_playerData *playerData);
int   clear_not_reject_player(t_playerData *playerData);
int   update_player_data(t_playerData *playerData);

//utils.c
void        view_board(void);
const char  *get_team_color(int team_no);
int         check_argument(int argc, const char *argv[], int *team_no);
int         check_first_step_flag(int flag, int team_no);

#endif