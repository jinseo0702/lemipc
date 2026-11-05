#include "../include/system.h"
#include "../include/logic.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/sem.h>


/* The shared-memory key names the generation. Only its creator makes the
 * companion objects. Keep the mutex closed until the board and queue are ready. */
static int join_ipcs(t_playerData *player, key_t key)
{
    player->mid = shmget(key, sizeof(t_shm), 0);
    if (player->mid == -1)
        return -1;
    for (int retry = 0; retry < 1000; ++retry) {
        player->sid = semget(key, 0, 0);
        player->qid = msgget(key, 0);
        if (shmget(key, sizeof(t_shm), 0) != player->mid)
            break;
        if (player->sid != -1 && player->qid != -1)
            return 0;
        usleep(1000);
    }
    return -1;
}

int init_ipcs_ids(t_playerData *player, int initialize_game)
{
    key_t key = ftok(PATHNAME, PROJID);

    if (key == (key_t)-1)
        return -1;
    player->mid = check_first_player(key);
    if (player->mid == -1)
        return errno == EEXIST ? join_ipcs(player, key) : -1;
    /* Index 0 is the mutex. Index 1 counts registered player processes. */
    player->sid = semget(key, 2, 0666 | IPC_CREAT | IPC_EXCL);
    player->qid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
    if (player->sid == -1 || player->qid == -1
        || (initialize_game && init_shm_and_pos_make(player) == -1)
        || semctl(player->sid, 0, SETVAL, 1) == -1) {
        clear_ipcs(player->mid, player->sid, player->qid);
        return -1;
    }
    return 2;
}

int adjust_player_count(int sid, short delta)
{
    struct sembuf op = {1, delta, SEM_UNDO | IPC_NOWAIT};

    while (semop(sid, &op, 1) == -1) {
        if (errno != EINTR)
            return -1;
    }
    return semctl(sid, 1, GETVAL);
}

int		check_first_player(key_t key){
  int check_shm_flag = 0;
  check_shm_flag = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT | IPC_EXCL);
  return (check_shm_flag);
}

void	clear_ipcs(int shm_id, int sem_id, int msg_id){
  if (sem_id != -1) {
    semctl(sem_id, 0, IPC_RMID, NULL);
  }
  if (msg_id != -1) {
    msgctl(msg_id, IPC_RMID, NULL);
  }
  if (shm_id != -1) {
    shmctl(shm_id, IPC_RMID, NULL);
  }
}

int		lock_sem(int semid){
  struct sembuf sembuf = {0, -1, SEM_UNDO};
  while (semop(semid, &sembuf, 1) == -1) {
    if (errno == EINTR) continue;
    perror("semop : lock failed");
    return (-1);
  }
  return (0);
}

int		unlock_sem(int semid){
  struct sembuf sembuf = {0, 1, SEM_UNDO};
  while (semop(semid, &sembuf, 1) == -1) {
    if (errno == EINTR) continue;
    perror("semop : unlock failed");
    return (-1);
  }
  return (0);
}

t_shm	*readonly_board(int shmid){
  t_shm *temp = (t_shm *)shmat(shmid, NULL, SHM_RDONLY);
  if ((void *)temp == (void *)-1) {
    perror("shmat : readonly_board failed");
    return (NULL);
  }
  return (temp);
}

t_shm	*readwrite_board(int shmid){
  t_shm *temp = (t_shm *)shmat(shmid, NULL, 0);
  if ((void *)temp == (void *)-1) {
    perror("shmat : readwrite_board failed");
    return (NULL);
  }
  return (temp);
}

int		detach_board(t_shm *board){
  if (shmdt((void *)board) == -1) {
    perror("shmdt : failed");
    return (-1);
  }
  return (0);
}

int		send_msg(int qid, t_myMsgbuf *msgbuf){
  if (msgsnd(qid, msgbuf, sizeof(t_myMsgbuf) - sizeof(long), IPC_NOWAIT) == -1) {
    perror("msgsnd : failed");
    return (-1);
  }
  return (0);
}

int		recv_msg(int qid, t_myMsgbuf *msgbuf, long type){
  if (msgrcv(qid, msgbuf, sizeof(t_myMsgbuf) - sizeof(long), type, IPC_NOWAIT) == -1) {
    if (errno == ENOMSG) return (ENOMSG);
    perror("msgrcv : failed");
    return (-1);
  }
  return (0);
}
