#include "../include/game_algorithm.h"
#include "../include/logic.h"
#include "../include/system.h"
#include <stdlib.h>
#include <unistd.h>

static void	init_game_state(t_game_state *st)
{
	st->die_flag = 0;
	st->end_flag = 0;
	st->msg_flag = 0;
	st->track_flag = 0;
	st->random_flag = 0;
	st->hold_ticks = 0;
	st->force_random_ticks = 0;
	st->target.x = -1;
	st->target.y = -1;
}

static void	reset_loop_flags(t_game_state *st)
{
	st->die_flag = 0;
	st->end_flag = 0;
	st->msg_flag = 0;
	st->random_flag = 0;
}

static int	calculate_distance_sq(int x1, int y1, int x2, int y2)
{
	return ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

static int	check_alone_on_board(t_playerData *pd)
{
	int	team_count;
	int	i;

	team_count = 0;
	i = 0;
	while (i < MAXTEAM)
	{
		if (pd->readonly->team_nbs[i] > 0)
			team_count++;
		i++;
	}
	if (team_count <= 1)
		return (1);
	return (0);
}

static int	check_surrounded(t_playerData *pd)
{
	int		x;
	int		y;
	char	up;
	char	down;
	char	left;
	char	right;

	x = pd->x;
	y = pd->y;
	up = 0;
	down = 0;
	left = 0;
	right = 0;
	if (x > 0)
		up = pd->readonly->board[x - 1][y];
	if (x < HEIGHT - 1)
		down = pd->readonly->board[x + 1][y];
	if (y > 0)
		left = pd->readonly->board[x][y - 1];
	if (y < WIDTH - 1)
		right = pd->readonly->board[x][y + 1];
	if (up != 0 && up != '0' && up != pd->team_no
		&& down != 0 && down != '0' && down != pd->team_no
		&& up == down)
		return (1);
	if (left != 0 && left != '0' && left != pd->team_no
		&& right != 0 && right != '0' && right != pd->team_no
		&& left == right)
		return (1);
	return (0);
}

static int	apply_step(t_playerData *pd, int nx, int ny, t_pos *bump_pos)
{
	t_pos	old_pos;

	if (nx < 0 || nx >= HEIGHT || ny < 0 || ny >= WIDTH)
		return (-1);
	if (pd->readwrite->board[nx][ny] != '0')
	{
		bump_pos->x = nx;
		bump_pos->y = ny;
		return (0);
	}
	old_pos.x = pd->x;
	old_pos.y = pd->y;
	pd->readwrite->board[old_pos.x][old_pos.y] = '0';
	pd->x = nx;
	pd->y = ny;
	pd->readwrite->board[pd->x][pd->y] = pd->team_no;
	return (1);
}

static int	apply_greedy_move(t_playerData *pd, t_pos target, t_pos *bump_pos)
{
	int	dx;
	int	dy;
	int	nx;
	int	ny;

	if (target.x < 0 || target.y < 0)
		return (-1);
	dx = target.x - pd->x;
	dy = target.y - pd->y;
	if (dx == 0 && dy == 0)
		return (1);
	nx = pd->x;
	ny = pd->y;
	if (abs(dy) >= abs(dx))
	{
		if (dy > 0)
			ny = pd->y + 1;
		else
			ny = pd->y - 1;
	}
	else
	{
		if (dx > 0)
			nx = pd->x + 1;
		else
			nx = pd->x - 1;
	}
	return (apply_step(pd, nx, ny, bump_pos));
}

static int	move_random_direction(t_playerData *pd, t_pos *bump_pos)
{
	int	dirs[8][2];
	int	rand_idx;
	int	nx;
	int	ny;

	dirs[0][0] = -1; dirs[0][1] = 0;
	dirs[1][0] = 1;  dirs[1][1] = 0;
	dirs[2][0] = 0;  dirs[2][1] = -1;
	dirs[3][0] = 0;  dirs[3][1] = 1;
	dirs[4][0] = -1; dirs[4][1] = -1;
	dirs[5][0] = -1; dirs[5][1] = 1;
	dirs[6][0] = 1;  dirs[6][1] = -1;
	dirs[7][0] = 1;  dirs[7][1] = 1;
	rand_idx = rand() % 8;
	nx = pd->x + dirs[rand_idx][0];
	ny = pd->y + dirs[rand_idx][1];
	return (apply_step(pd, nx, ny, bump_pos));
}

static void	send_chase_message(t_playerData *pd, t_game_state *st)
{
	t_myMsgbuf	msgbuf;

	msgbuf.mytype = pd->team_no;
	msgbuf.x = st->target.x;
	msgbuf.y = st->target.y;
	msgbuf.kind = BROADCAST;
	msgbuf.team_no = pd->team_no;
	msgbuf.msg_order = DO_NOTHING;
	send_msg(pd->qid, &msgbuf);
}

static void	send_death_message(t_playerData *pd)
{
	t_myMsgbuf	msgbuf;

	msgbuf.mytype = pd->team_no;
	msgbuf.x = pd->x;
	msgbuf.y = pd->y;
	msgbuf.kind = DEATH;
	msgbuf.team_no = pd->team_no;
	msgbuf.msg_order = DO_NOTHING;
	send_msg(pd->qid, &msgbuf);
}

static int	recv_team_msg(t_playerData *pd, t_myMsgbuf *msgbuf)
{
	int	error;

	error = recv_msg(pd->qid, msgbuf, pd->team_no);
	if (error == ENOMSG)
		return (0);
	if (error == -1)
		return (-1);
	return (1);
}

static void	run_swarm_intelligence(t_playerData *pd, t_game_state *st)
{
	t_myMsgbuf	msgbuf;
	int			recv_result;
	int			dist;

	recv_result = recv_team_msg(pd, &msgbuf);
	if (recv_result == 1)
	{
		dist = calculate_distance_sq(pd->x, pd->y, msgbuf.x, msgbuf.y);
		if (dist <= 9)
		{
			st->target.x = msgbuf.x;
			st->target.y = msgbuf.y;
			st->track_flag = 1;
		}
		else if (!st->track_flag)
			st->random_flag = 1;
	}
	else if (!st->track_flag)
		st->random_flag = 1;
}

static void	process_death(t_playerData *pd, t_game_state *st)
{
	st->die_flag = 1;
	st->msg_flag = 1;
	pd->readwrite->board[pd->x][pd->y] = '0';
	pd->readwrite->player_nbs--;
	pd->readwrite->team_nbs[pd->team_no - 'A']--;
}

static void	clear_tracking(t_game_state *st)
{
	st->track_flag = 0;
	st->target.x = -1;
	st->target.y = -1;
}

static void	process_movement(t_playerData *pd, t_game_state *st)
{
	t_pos	bump_pos;
	int		result;

	bump_pos.x = -1;
	bump_pos.y = -1;
	if (st->hold_ticks > 0)
	{
		st->hold_ticks--;
		return ;
	}
	if (st->force_random_ticks > 0)
	{
		st->force_random_ticks--;
		clear_tracking(st);
		st->random_flag = 1;
	}
	if (st->track_flag)
	{
		result = apply_greedy_move(pd, st->target, &bump_pos);
		if (result == 0)
		{
			st->target = bump_pos;
			st->msg_flag = 1;
			st->hold_ticks = 2;
			st->force_random_ticks = 1;
			st->track_flag = 0;
			return ;
		}
		if (pd->x == st->target.x && pd->y == st->target.y)
		{
			clear_tracking(st);
			st->random_flag = 1;
		}
		else if (result == -1 && !st->random_flag)
			st->random_flag = 1;
	}
	if (st->random_flag || !st->track_flag)
	{
		result = move_random_direction(pd, &bump_pos);
		if (result == 0)
		{
			st->target = bump_pos;
			st->msg_flag = 1;
			st->hold_ticks = 2;
			st->force_random_ticks = 1;
			return ;
		}
	}
}

static int	process_post_unlock(t_playerData *pd, t_game_state *st)
{
	if (st->end_flag)
	{
		terminate_player(pd);
		return (1);
	}
	if (st->msg_flag)
	{
		if (st->die_flag)
		{
			send_death_message(pd);
			if (pd->readonly != NULL)
				detach_board(pd->readonly);
			if (pd->readwrite != NULL)
				detach_board(pd->readwrite);
			return (1);
		}
		send_chase_message(pd, st);
	}
	return (0);
}

int	run_game_loop(t_playerData *pd)
{
	t_game_state	st;

	srand(time(NULL) ^ (pd->team_no * 1000 + pd->team_mb_cnt));
	init_game_state(&st);
	while (1)
	{
		sig_is_come(pd);
		if (pd->readonly->game_state == END_GAME){
			terminate_player(pd);
			return (0);
		}
		reset_loop_flags(&st);
		run_swarm_intelligence(pd, &st);
		if (lock_sem(pd->sid) == -1)
			return (-1);
		if (check_alone_on_board(pd))
			st.end_flag = 1;
		else if (check_surrounded(pd))
			process_death(pd, &st);
		else
			process_movement(pd, &st);
		if (unlock_sem(pd->sid) == -1)
			return (-1);
		if (process_post_unlock(pd, &st))
			return (0);
		usleep(100000);
	}
	return (0);
}

void	terminate_player(t_playerData *pd)
{
	if (pd->readonly != NULL)
		detach_board(pd->readonly);
	if (pd->readwrite != NULL)
		detach_board(pd->readwrite);
	pd->readonly = NULL;
	pd->readwrite = NULL;

	sleep(1);
	struct shmid_ds ds;
	shmctl(pd->mid, IPC_STAT, &ds);
	if (ds.shm_nattch == 0) {
		clear_ipcs(pd->mid, pd->sid, pd->qid);
	}
}
