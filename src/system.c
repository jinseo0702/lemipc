#include "../include/system.h"
#include <stdio.h>
#include <sys/sem.h>


int		init_ipcs_ids(t_playerData *playerData){
  key_t key = ftok(PATHNAME, PROJID);
  int first_flag = 0;
  if (key == -1) {
    perror("ftok : failed");
    return (-1);
  }
  playerData->mid = check_first_player(key);
  if (playerData->mid == -1){
    playerData->mid = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT);
  }
  else{
    first_flag = 1;
  }
  playerData->sid = semget(key, 1, 0666 | IPC_CREAT);
  playerData->qid = msgget(key, 0666 | IPC_CREAT);
  if (playerData->qid == -1 || playerData->sid == -1 || playerData->mid == -1) {
    clear_ipcs(playerData->qid, playerData->sid, playerData->mid);
    return (-1);
  }
  if (first_flag == 1){
    union semun {
      int              val;  
      struct semid_ds *buf;  
      unsigned short  *array;
      struct seminfo  *__buf;
    } arg;
    arg.val = 1;
    if(semctl(playerData->sid, 0, SETVAL, arg) == -1){
      perror("semctl: sem init fatal");
      return (-1); 
    }
  }
  return (1);
}

int		check_first_player(key_t key){
  int check_shm_flag = 0;
  check_shm_flag = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT | IPC_EXCL);
  return (check_shm_flag);
}

void	clear_ipcs(int shm_id, int sem_id, int msg_id){
  if (shm_id != -1) {
    shmctl(shm_id, IPC_RMID, NULL);
  }
  if (sem_id != -1) {
    semctl(sem_id, 0, IPC_RMID, NULL);
  }
  if (msg_id != -1) {
    msgctl(msg_id, IPC_RMID, NULL);
  }
}

int		lock_sem(int semid){
  struct sembuf sembuf = {0, -1, 0};
  if (semop(semid, &sembuf, 1) == -1) {
    perror("semop : lock failed");
    return (-1);
  }
  return (0);
}

int		unlock_sem(int semid){
  struct sembuf sembuf = {0, 1, 0};
  if (semop(semid, &sembuf, 1) == -1) {
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
  if (msgsnd(qid, msgbuf, sizeof(t_myMsgbuf), IPC_NOWAIT) == -1) {
    perror("msgsnd : failed");
    return (-1);
  }
  return (0);
}

int		recv_msg(int qid, t_myMsgbuf *msgbuf, long type){
  if (msgrcv(qid, msgbuf, sizeof(t_myMsgbuf), type, IPC_NOWAIT) == -1) {
    perror("msgrcv : failed");
    return (-1);
  }
  return (0);
}