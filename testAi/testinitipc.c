#include <string.h>
#include <sys/ipc.h> 
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct s_playerData{
	int team_no;
	int x;
	int y;
	int t_key;
	int team_mb_cnt; //if num 1 is failer game
	int qid; //msg queue id
	int sid; //semaphore id
	int mid; //shm id
}t_playerData;

typedef struct s_shm {
	int height;
	int width;
	int player_nbs;
	int board[HEIGHT][WIDTH];
} t_shm;

typedef struct s_myMsgbuf{
    long mytype;
    int kind;
}t_myMsgbuf;

void clean_ipcs(int shm_id, int sem_id, int msg_id);

int init_ipcs_ids(t_playerData *playerData){
    t_myMsgbuf msgbuf;
    key_t key = ftok(".", 'A');
    if (key == -1) {
      perror("ftok : failed");
      exit(EXIT_FAILURE);
    }
    playerData->qid = msgget(key, 0666 | IPC_CREAT);
    playerData->sid = semget(key, 1, 0666 | IPC_CREAT);
    playerData->mid = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT);
    if (playerData->qid == -1 || playerData->sid == -1 || playerData->mid == -1) {
      clean_ipcs(playerData->qid, playerData->sid, playerData->mid);
      return (-1);
    }
    return (1);
}

int check_first_player(t_playerData *playerData, key_t key){
  int check_shm_flag = 0;
  check_shm_flag = shmget(key, sizeof(t_shm), 0666 | IPC_CREAT | IPC_EXCL);
  if (check_shm_flag == -1) {
    return (0); // is not first player
  }
  playerData->mid = check_shm_flag;
  return (1); // is first player
}

void clean_ipcs(int shm_id, int sem_id, int msg_id){
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

/*
//sembuf = {0, -1, 0} is lock
int
lock_sem(int semid);
*/
int lock_sem(int semid){
  struct sembuf sembuf = {0, -1, 0};
  if (semop(semid, &sembuf, 1) == -1) {
    perror("semop : lock failed");
    return (-1);
  }
  return (0);
}
/*
//sembuf = {0, 1, 0} is unlock
int
unlock_sem(int semid);
*/
int unlock_sem(int semid){
  struct sembuf sembuf = {0, 1, 0};
  if (semop(semid, &sembuf, 1) == -1) {
    perror("semop : unlock failed");
    return (-1);
  }
  return (0);
}

t_shm *readonly_board(int shmid){
  t_shm *temp = (t_shm *)shmat(shmid, NULL, SHM_RDONLY);
  if ((void *)temp == (void *)-1) {
    perror("shmat : failed");
    return (NULL);
  }
  return (temp);
}

t_shm *readwrite_board(int shmid){
  t_shm *temp = (t_shm *)shmat(shmid, NULL, 0);
  if ((void *)temp == (void *)-1) {
    perror("shmat : failed");
    return (NULL);
  }
  return (temp);
}

int detach_board(t_shm *board){
  if (shmdt((void *)board) == -1) {
    perror("shmdt : failed");
    return (-1);
  }
  return (0);
}

int send_msg(int qid, t_myMsgbuf *msgbuf){
  if (msgsnd(qid, msgbuf, sizeof(t_myMsgbuf), IPC_NOWAIT) == -1) {
    perror("msgsnd : failed");
    return (-1);
  }
  return (0);
}

int recv_msg(int qid, t_myMsgbuf *msgbuf, long type){

}

int main(){
    init_ipc();
    return 0;
}