#include "../../include/data.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};

static key_t get_key(void)
{
	key_t key;

	key = ftok(PATHNAME, PROJID);
	if (key == (key_t)-1)
	{
		perror("ftok");
		exit(2);
	}
	return (key);
}

static void get_ids(key_t key, int *mid, int *sid, int *qid)
{
	*mid = shmget(key, sizeof(t_shm), 0);
	*sid = semget(key, 1, 0);
	*qid = msgget(key, 0);
}

static int cleanup_ids(int mid, int sid, int qid)
{
	int failed;

	failed = 0;
	if (mid != -1 && shmctl(mid, IPC_RMID, NULL) == -1)
		failed = 1;
	if (sid != -1 && semctl(sid, 0, IPC_RMID) == -1)
		failed = 1;
	if (qid != -1 && msgctl(qid, IPC_RMID, NULL) == -1)
		failed = 1;
	return (failed);
}

static int cmd_cleanup(void)
{
	int mid;
	int sid;
	int qid;

	get_ids(get_key(), &mid, &sid, &qid);
	printf("{\"removed\":{\"shm\":%s,\"sem\":%s,\"msg\":%s}}\n",
		mid == -1 ? "false" : "true",
		sid == -1 ? "false" : "true",
		qid == -1 ? "false" : "true");
	return (cleanup_ids(mid, sid, qid));
}

static int cmd_snapshot(void)
{
	key_t key;
	int mid;
	int sid;
	int qid;
	int semval;
	unsigned long nattch;
	unsigned long qnum;
	struct shmid_ds shm_ds;
	struct msqid_ds msg_ds;
	t_shm *shm;
	int a_count;
	int b_count;
	int empty_count;
	int other_count;

	key = get_key();
	get_ids(key, &mid, &sid, &qid);
	semval = -1;
	nattch = 0;
	qnum = 0;
	if (sid != -1)
		semval = semctl(sid, 0, GETVAL);
	if (mid != -1 && shmctl(mid, IPC_STAT, &shm_ds) == 0)
		nattch = (unsigned long)shm_ds.shm_nattch;
	if (qid != -1 && msgctl(qid, IPC_STAT, &msg_ds) == 0)
		qnum = (unsigned long)msg_ds.msg_qnum;
	printf("{\"key\":%ld,\"shm\":%s,\"sem\":%s,\"msg\":%s,",
		(long)key, mid == -1 ? "false" : "true",
		sid == -1 ? "false" : "true", qid == -1 ? "false" : "true");
	printf("\"mid\":%d,\"sid\":%d,\"qid\":%d,\"semval\":%d,",
		mid, sid, qid, semval);
	printf("\"nattch\":%lu,\"qnum\":%lu", nattch, qnum);
	if (mid == -1)
	{
		printf("}\n");
		return (0);
	}
	shm = shmat(mid, NULL, SHM_RDONLY);
	if (shm == (void *)-1)
	{
		printf(",\"attach_error\":%d}\n", errno);
		return (1);
	}
	a_count = 0;
	b_count = 0;
	empty_count = 0;
	other_count = 0;
	for (int x = 0; x < HEIGHT; ++x)
	{
		for (int y = 0; y < WIDTH; ++y)
		{
			if (shm->board[x][y] == 'A')
				a_count++;
			else if (shm->board[x][y] == 'B')
				b_count++;
			else if (shm->board[x][y] == '0')
				empty_count++;
			else
				other_count++;
		}
	}
	printf(",\"state\":%d,\"players\":%d,\"teams\":[%d,%d]",
		shm->game_state, shm->player_nbs, shm->team_nbs[0], shm->team_nbs[1]);
	printf(",\"cells\":{\"A\":%d,\"B\":%d,\"empty\":%d,\"other\":%d}",
		a_count, b_count, empty_count, other_count);
	printf(",\"board\":\"");
	for (int x = 0; x < HEIGHT; ++x)
		for (int y = 0; y < WIDTH; ++y)
		{
			unsigned char c = (unsigned char)shm->board[x][y];
			if (c >= 0x20 && c <= 0x7e && c != '\"' && c != '\\')
				putchar(c);
			else
				printf("\\u%04x", c);
		}
	printf("\"}\n");
	if (shmdt(shm) == -1)
		return (1);
	return (0);
}

static int cmd_create(int argc, char **argv)
{
	key_t key;
	int mid;
	int sid;
	int qid;
	union semun arg;
	t_shm *shm;

	if (argc != 6)
		return (2);
	key = get_key();
	mid = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT | IPC_EXCL);
	if (mid == -1)
		return (perror("shmget"), 1);
	sid = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
	if (sid == -1)
		return (perror("semget"), cleanup_ids(mid, -1, -1), 1);
	qid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
	if (qid == -1)
		return (perror("msgget"), cleanup_ids(mid, sid, -1), 1);
	arg.val = 1;
	if (semctl(sid, 0, SETVAL, arg) == -1)
		return (perror("semctl"), cleanup_ids(mid, sid, qid), 1);
	shm = shmat(mid, NULL, 0);
	if (shm == (void *)-1)
		return (perror("shmat"), cleanup_ids(mid, sid, qid), 1);
	memset(shm, 0, sizeof(*shm));
	memset(shm->board, '0', sizeof(shm->board));
	shm->game_state = atoi(argv[2]);
	shm->player_nbs = atoi(argv[3]);
	shm->team_nbs[0] = atoi(argv[4]);
	shm->team_nbs[1] = atoi(argv[5]);
	if (shmdt(shm) == -1)
		return (perror("shmdt"), cleanup_ids(mid, sid, qid), 1);
	printf("{\"created\":true,\"mid\":%d,\"sid\":%d,\"qid\":%d}\n",
		mid, sid, qid);
	return (0);
}

static int cmd_hold_lock(int argc, char **argv)
{
	int mid;
	int sid;
	int qid;
	long millis;
	struct sembuf op;
	struct timespec duration;

	if (argc != 3)
		return (2);
	get_ids(get_key(), &mid, &sid, &qid);
	(void)mid;
	(void)qid;
	if (sid == -1)
		return (1);
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = SEM_UNDO;
	if (semop(sid, &op, 1) == -1)
		return (perror("semop"), 1);
	printf("LOCKED\n");
	fflush(stdout);
	millis = strtol(argv[2], NULL, 10);
	duration.tv_sec = millis / 1000;
	duration.tv_nsec = (millis % 1000) * 1000000L;
	nanosleep(&duration, NULL);
	return (0);
}

static int cmd_send(int argc, char **argv)
{
	int mid;
	int sid;
	int qid;
	t_myMsgbuf msg;

	if (argc != 8)
		return (2);
	get_ids(get_key(), &mid, &sid, &qid);
	(void)mid;
	(void)sid;
	if (qid == -1)
		return (1);
	msg.mytype = strtol(argv[2], NULL, 10);
	msg.x = atoi(argv[3]);
	msg.y = atoi(argv[4]);
	msg.kind = (e_kind)atoi(argv[5]);
	msg.team_no = (e_team_no)atoi(argv[6]);
	msg.msg_order = (e_msg_order)atoi(argv[7]);
	if (msgsnd(qid, &msg, sizeof(msg) - sizeof(long), IPC_NOWAIT) == -1)
		return (perror("msgsnd"), 1);
	return (0);
}

int main(int argc, char **argv)
{
	if (argc < 2)
		return (2);
	if (strcmp(argv[1], "cleanup") == 0)
		return (cmd_cleanup());
	if (strcmp(argv[1], "snapshot") == 0)
		return (cmd_snapshot());
	if (strcmp(argv[1], "create") == 0)
		return (cmd_create(argc, argv));
	if (strcmp(argv[1], "hold-lock") == 0)
		return (cmd_hold_lock(argc, argv));
	if (strcmp(argv[1], "send") == 0)
		return (cmd_send(argc, argv));
	fprintf(stderr, "unknown command\n");
	return (2);
}
