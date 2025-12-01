#include "../include/system.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void init_playerdatata(t_playerData *data);
int test_init_and_clear(void);
int test_check_fist_player(void);
int test_shm_player(void);
int test_sem(void);
int test_sem_clear_ipcs(void);
int test_msq_player(void);

void init_playerdatata(t_playerData *data){
  data->mid = -1;
  data->sid = -1;
  data->qid = -1;
  data->t_key = -1;
  data->team_mb_cnt = 0;
  data->team_no = 0;
  data->x = 0;
  data->y = 0;
}

int test_init_and_clear(void){
  printf("------------------test_init_and_clear------------------\n");
  t_playerData player;

  init_playerdatata(&player);
  if (init_ipcs_ids(&player) == -1) {
    return (1);
  }
  clear_ipcs(player.mid, player.sid, player.qid);
  return (0);
}

int test_check_fist_player(void){
  printf("------------------test_check_fist_player------------------\n");
  pid_t pid = 0;
  pid = fork();
  if (pid == 0){
    printf("It's fist player is run\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      exit(1);
    }
    exit (0);
  }
  else if (pid == -1){
    perror("fork : failed");
    return (1);
  }
  else {
    int wstatus;
    if(wait(&wstatus) == -1){
      perror("wait : is faild");
      return (1);
    }
    else if (WIFEXITED(wstatus)) {
      int flag = WEXITSTATUS(wstatus);
      if (flag == 1){
        perror("child : Fatal Error");
        return (1);
      }
      else {
        printf("exit code=%d\n", WEXITSTATUS(wstatus));
      }
    }
    else {
      perror("child : Fatal Error");
      return (1);
    }
    printf("It's second player is run\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      return(1);
    }
    clear_ipcs(player.mid, player.sid, player.qid);
    return (0);
  }
}

int test_shm_player(void){
  printf("------------------test_shm_player------------------\n");
  pid_t pid = 0;
  pid = fork();
  if (pid == 0){
    printf("It's fist player is run\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      exit(1);
    }
    t_shm *readonly = readonly_board(player.mid);
    t_shm *readwrite = readwrite_board(player.mid);
    if (readonly == NULL || readwrite == NULL){
      exit(1);
    }
    printf("readonly addr is %p \n", readonly);
    printf("readwrite addr is %p \n", readwrite);
    printf("first board num is %d \n", readonly->player_nbs);
    lock_sem(player.sid);
    readwrite->player_nbs += 1;
    unlock_sem(player.sid);
    if (detach_board(readonly) == -1) {
      exit(1);
    }
    if (detach_board(readwrite) == -1) {
      exit(1);
    }
    exit (0);
  }
  else if (pid == -1){
    perror("fork : failed");
    return (1);
  }
  else {
    int wstatus;
    if(wait(&wstatus) == -1){
      perror("wait : is faild");
      return (1);
    }
    else if (WIFEXITED(wstatus)) {
      int flag = WEXITSTATUS(wstatus);
      if (flag == 1){
        perror("child : Fatal Error");
        return (1);
      }
      else {
        printf("exit code=%d\n", WEXITSTATUS(wstatus));
      }
    }
    else {
      perror("child : Fatal Error");
      return (1);
    }
    printf("It's second player is run\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      return(1);
    }
    t_shm *readonly = readonly_board(player.mid);
    t_shm *readwrite = readwrite_board(player.mid);
    if (readonly == NULL || readwrite == NULL){
      exit(1);
    }
    printf("second board num is %d \n", readonly->player_nbs);
    lock_sem(player.sid);
    readwrite->player_nbs += 1;
    unlock_sem(player.sid);
    printf("second board num is %d \n", readonly->player_nbs);
    clear_ipcs(player.mid, player.sid, player.qid);
    return (0);
  }
}

int test_sem(void){
  printf("------------------test_sem------------------\n");
  t_playerData player;
  init_playerdatata(&player);
  if (init_ipcs_ids(&player) == -1) {
    return (1);
  }
  t_shm *ptr = readwrite_board(player.mid);
  if (ptr == NULL){
    return (1);
  }
  if (lock_sem(player.sid) == -1) { perror("lock_sem"); return 1; }
  if (ptr->player_nbs == 0){
    ptr->player_nbs++;
  }
  if (unlock_sem(player.sid) == -1) { perror("unlock_sem"); return 1; }
  while (1) {
    if (lock_sem(player.sid) == -1) { perror("lock_sem"); return 1; }
    if (ptr->player_nbs == 1000){
      if (unlock_sem(player.sid) == -1) { perror("unlock_sem"); return 1; }
      break;
    }
    ptr->player_nbs++;
    if (unlock_sem(player.sid) == -1) { perror("unlock_sem"); return 1; }
  }
  printf("pid %d ptr->player_nbs is %d\n", getpid(), ptr->player_nbs);
  return (0);
}

int test_sem_clear_ipcs(void){
  t_playerData player;
  init_playerdatata(&player);
  if (init_ipcs_ids(&player) == -1) {
    return (1);
  }
  clear_ipcs(player.mid, player.sid, player.qid);
  return (0);
}

int test_msq_player(void){
  printf("------------------test_msq_player------------------\n");
  pid_t pid = 0;
  pid = fork();
  if (pid == 0){
    printf("It's fist player is send msg\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      exit(1);
    }

    t_myMsgbuf msgbug;
    msgbug.mytyep = 0;
    msgbug.x = 0;
    msgbug.y = 1;
    msgbug.kind = START_GAME;
    msgbug.team_no = NON;
    msgbug.msg_order = DO_NOTHING;
    if (send_msg(player.qid, &msgbug) == -1){
      exit(1);
    }
  
    exit (0);
  }
  else if (pid == -1){
    perror("fork : failed");
    return (1);
  }
  else {
    int wstatus;
    if(wait(&wstatus) == -1){
      perror("wait : is faild");
      return (1);
    }
    else if (WIFEXITED(wstatus)) {
      int flag = WEXITSTATUS(wstatus);
      if (flag == 1){
        perror("child : Fatal Error");
        return (1);
      }
      else {
        printf("exit code=%d\n", WEXITSTATUS(wstatus));
      }
    }
    else {
      perror("child : Fatal Error");
      return (1);
    }
    printf("It's second player is run\n");
    t_playerData player;
    init_playerdatata(&player);
    if (init_ipcs_ids(&player) == -1) {
      return(1);
    }

    clear_ipcs(player.mid, player.sid, player.qid);
    return (0);
  }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <test_name>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "test_init_and_clear") == 0)
        return test_init_and_clear();
    if (strcmp(argv[1], "test_check_fist_player") == 0)
        return test_check_fist_player();
    if (strcmp(argv[1], "test_shm_player") == 0)
        return test_shm_player();
    if (strcmp(argv[1], "test_sem") == 0)
        return test_sem();
    if (strcmp(argv[1], "test_sem_clear_ipcs") == 0)
        return test_sem_clear_ipcs();

    fprintf(stderr, "Unknown test: %s\n", argv[1]);
    return 1;
}