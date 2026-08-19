#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game_algorithm.c"

static int g_recv_result;
static t_myMsgbuf g_recv_message;
static int g_send_count;
static t_myMsgbuf g_sent_message;

int recv_msg(int qid, t_myMsgbuf *msgbuf, long type)
{
	(void)qid;
	(void)type;
	if (g_recv_result == 0)
		*msgbuf = g_recv_message;
	return (g_recv_result);
}

int send_msg(int qid, t_myMsgbuf *msgbuf)
{
	(void)qid;
	g_send_count++;
	g_sent_message = *msgbuf;
	return (0);
}

void sig_is_come(t_playerData *pd)
{
	(void)pd;
}

int lock_sem(int sid)
{
	(void)sid;
	return (0);
}

int unlock_sem(int sid)
{
	(void)sid;
	return (0);
}

int detach_board(t_shm *board)
{
	(void)board;
	return (0);
}

void clear_ipcs(int mid, int sid, int qid)
{
	(void)mid;
	(void)sid;
	(void)qid;
}

static void setup_player(t_playerData *pd, t_shm *shm, int x, int y, int team)
{
	memset(pd, 0, sizeof(*pd));
	memset(shm, 0, sizeof(*shm));
	memset(shm->board, '0', sizeof(shm->board));
	pd->x = x;
	pd->y = y;
	pd->team_no = team;
	pd->readonly = shm;
	pd->readwrite = shm;
	shm->board[x][y] = team;
}

static int case_distance(void)
{
	return (!(calculate_distance_sq(0, 0, 3, 0) == 9
		&& calculate_distance_sq(0, 0, 3, 1) == 10));
}

static int case_greedy_y_tie(void)
{
	t_playerData pd;
	t_shm shm;
	t_pos bump;
	int result;

	setup_player(&pd, &shm, 4, 4, 'A');
	result = apply_greedy_move(&pd, (t_pos){6, 6}, &bump);
	return (!(result == 1 && pd.x == 4 && pd.y == 5
		&& shm.board[4][4] == '0' && shm.board[4][5] == 'A'));
}

static int case_greedy_x(void)
{
	t_playerData pd;
	t_shm shm;
	t_pos bump;
	int result;

	setup_player(&pd, &shm, 4, 4, 'A');
	result = apply_greedy_move(&pd, (t_pos){1, 5}, &bump);
	return (!(result == 1 && pd.x == 3 && pd.y == 4));
}

static int case_collision(void)
{
	t_playerData pd;
	t_shm shm;
	t_pos bump;
	int result;

	setup_player(&pd, &shm, 4, 4, 'A');
	shm.board[4][5] = 'B';
	result = apply_greedy_move(&pd, (t_pos){4, 7}, &bump);
	return (!(result == 0 && pd.x == 4 && pd.y == 4
		&& bump.x == 4 && bump.y == 5));
}

static int case_random_step(void)
{
	t_playerData pd;
	t_shm shm;
	t_pos bump;
	int before_x;
	int before_y;
	int result;

	setup_player(&pd, &shm, 5, 5, 'A');
	before_x = pd.x;
	before_y = pd.y;
	srand(1);
	result = move_random_direction(&pd, &bump);
	return (!(result == 1 && abs(pd.x - before_x) <= 1
		&& abs(pd.y - before_y) <= 1
		&& !(pd.x == before_x && pd.y == before_y)));
}

static int case_surrounded_horizontal(void)
{
	t_playerData pd;
	t_shm shm;

	setup_player(&pd, &shm, 5, 5, 'A');
	shm.board[5][4] = 'B';
	shm.board[5][6] = 'B';
	return (check_surrounded(&pd) != 1);
}

static int case_surrounded_mixed(void)
{
	t_playerData pd;
	t_shm shm;

	setup_player(&pd, &shm, 5, 5, 'A');
	shm.board[5][4] = 'B';
	shm.board[5][6] = 'C';
	return (check_surrounded(&pd) != 0);
}

static int case_death_update(void)
{
	t_playerData pd;
	t_shm shm;
	t_game_state st;

	setup_player(&pd, &shm, 5, 5, 'A');
	shm.player_nbs = 4;
	shm.team_nbs[0] = 2;
	shm.team_nbs[1] = 2;
	init_game_state(&st);
	process_death(&pd, &st);
	return (!(st.die_flag == 1 && st.msg_flag == 1
		&& shm.board[5][5] == '0' && shm.player_nbs == 3
		&& shm.team_nbs[0] == 1));
}

static int case_out_of_radius(void)
{
	t_playerData pd;
	t_shm shm;
	t_game_state st;

	setup_player(&pd, &shm, 0, 0, 'A');
	init_game_state(&st);
	g_recv_result = 0;
	g_recv_message.x = 3;
	g_recv_message.y = 1;
	g_recv_message.kind = BROADCAST;
	run_swarm_intelligence(&pd, &st);
	return (!(st.track_flag == 0 && st.random_flag == 1));
}

static int case_message_kind(void)
{
	t_playerData pd;
	t_shm shm;
	t_game_state st;

	setup_player(&pd, &shm, 5, 5, 'A');
	init_game_state(&st);
	g_recv_result = 0;
	g_recv_message.x = 5;
	g_recv_message.y = 6;
	g_recv_message.kind = DEATH;
	g_recv_message.team_no = 'A';
	run_swarm_intelligence(&pd, &st);
	printf("{\"kind\":%d,\"track\":%d,\"target\":[%d,%d]}\n",
		g_recv_message.kind, st.track_flag, st.target.x, st.target.y);
	return (st.track_flag == 1 ? 0 : 1);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		return (2);
	if (strcmp(argv[1], "distance") == 0)
		return (case_distance());
	if (strcmp(argv[1], "greedy_y_tie") == 0)
		return (case_greedy_y_tie());
	if (strcmp(argv[1], "greedy_x") == 0)
		return (case_greedy_x());
	if (strcmp(argv[1], "collision") == 0)
		return (case_collision());
	if (strcmp(argv[1], "random_step") == 0)
		return (case_random_step());
	if (strcmp(argv[1], "surrounded_horizontal") == 0)
		return (case_surrounded_horizontal());
	if (strcmp(argv[1], "surrounded_mixed") == 0)
		return (case_surrounded_mixed());
	if (strcmp(argv[1], "death_update") == 0)
		return (case_death_update());
	if (strcmp(argv[1], "out_of_radius") == 0)
		return (case_out_of_radius());
	if (strcmp(argv[1], "message_kind") == 0)
		return (case_message_kind());
	return (2);
}
