#ifndef DATA_H
#define DATA_H

#define WIDTH 16
#define HEIGHT 16
#define MAXTEAM 9
#define MAXTEAMNB 5
#define PATHNAME "/tmp"
#define PROJID 'A'

  typedef enum e_kind {
    START_GAME = 0,
    DEATH = 1,
    END_GAME = 2,
  } e_kind;

  typedef enum e_team_no {
    NON = 7,
    TEAM_1 = 1,
    TEAM_2 = 2,
    TEAM_3 = 3,
    TEAM_4 = 4,
    TEAM_5 = 5,
  } e_team_no;

  typedef enum e_msg_order {
    KILL_PLAYER = 0,
    DO_NOTHING = 1,
  } e_msg_order;

typedef struct s_shm {
	int player_nbs;
	int board[HEIGHT][WIDTH];
	int team_nbs[MAXTEAM];
} t_shm;

typedef struct s_myMsgbuf{
	long mytyep; //1 전체채널, 2...n 팀채널
	int x;
	int y;
	e_kind kind; //0 게임 시작, 1 (x, y) 죽음, 2 게임끝
	e_team_no team_no;
	e_msg_order msg_order;
}t_myMsgbuf;

typedef struct s_playerData{
	int team_no;
	int x;
	int y;
	int t_key;
	int team_mb_cnt; //if num 1 is failer game
	int qid; //msg queue id
	int sid; //semaphore id
	int mid; //shm id
	t_shm *readonly;
	t_shm *readwrite;
}t_playerData;

#endif