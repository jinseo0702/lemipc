#ifndef DATA_H
#define DATA_H

#define WIDTH 16
#define HEIGHT 16
#define MAXTEAM 6
#define MAXTEAMNB 5
#define PATHNAME "/tmp"
#define PROJID 'A'

  typedef enum e_kind {
		BROADCAST = 10,
		INIT_PLAYER = 11,
    START_GAME,
    DEATH,
    END_GAME,
  } e_kind;

  typedef enum e_team_no {
		NONE = 0,
    TEAM_1,
    TEAM_2,
    TEAM_3,
    TEAM_4,
    TEAM_5,
		TEAM_6,
		TEAM_7,
		TEAM_8,
		TEAM_9,
  } e_team_no;

  typedef enum e_msg_order {
    KILL_PLAYER = 0,
    DO_NOTHING = 1,
  } e_msg_order;

typedef struct s_shm {
	int player_nbs;
	char board[HEIGHT][WIDTH];
	int team_nbs[MAXTEAM];
} t_shm;

typedef struct s_myMsgbuf{
	long mytype; //10 전체채널, 1...9 팀채널 
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

typedef struct s_pos {
	int x;
	int y;
} t_pos;

#endif