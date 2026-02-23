#ifndef SYSTEM_H
#define SYSTEM_H

#include "data.h"
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>


/*
success return 1 / fail return -1
use ftok msgget semget shmget
*/
int		init_ipcs_ids(t_playerData *playerData);
/*
check first player by (0666 | IPC_CREAT | IPC_EXCL)
return 1 is player is first player 0 is second player
*/
int		check_first_player(key_t key);
/*
clear ipcs resources
use shmctl, semctl, msgctl
*/
void	clear_ipcs(int shm_id, int sem_id, int msg_id);
//sembuf = {0, -1, 0} is lock
int		lock_sem(int semid);
//sembuf = {0, 1, 0} is unlock
int		unlock_sem(int semid);
t_shm	*readonly_board(int shmid);
t_shm	*readwrite_board(int shmid);
int		detach_board(t_shm *board);
int		send_msg(int qid, t_myMsgbuf *msgbuf);
int		recv_msg(int qid, t_myMsgbuf *msgbuf, long type);

#endif
