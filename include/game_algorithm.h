#ifndef GAME_ALGORITHM_H
#define GAME_ALGORITHM_H

#include "data.h"

typedef struct s_game_state {
	int die_flag;
	int end_flag;
	int msg_flag;
	int track_flag;
	int random_flag;
	int hold_ticks;
	int force_random_ticks;
	t_pos target;
} t_game_state;

int		run_game_loop(t_playerData *pd);
void	terminate_player(t_playerData *pd);

#endif
