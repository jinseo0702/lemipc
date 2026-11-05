#include "../include/logic.h"
#include "../include/system.h"
#include "../libft/libft.h"

int init_shm_and_pos_make(t_playerData *playerData){
  if (attach_shm(playerData) == -1) return (-1);
  playerData->readwrite->game_state = RECRUITMENT;
  ft_memset(playerData->readwrite->board, 48, sizeof(playerData->readwrite->board));
  ft_memset(playerData->readwrite->team_nbs, 0, sizeof(playerData->readwrite->team_nbs));
  t_pos pos[WIDTH * HEIGHT];
  make_rand_position(pos);
  int max_cnt = MAXTEAMNB * MAXTEAM;
  t_myMsgbuf msgbuf;
  msgbuf.mytype = BROADCAST;
  for (int i = 0; i < max_cnt; i++){
    msgbuf.x = pos[i].x;
    msgbuf.y = pos[i].y;
    msgbuf.kind = INIT_PLAYER;
    msgbuf.team_no = playerData->team_no;
    if (send_msg(playerData->qid, &msgbuf) == -1) return (-1);
  }
	return (0);
}

/* Called with the mutex held. Publish the position, membership and start
 * barrier as one admission so a peer cannot tear down or start midway. */
int update_player_data(t_playerData *player)
{
    t_shm *game = player->readwrite;
    int team = player->team_no - 'A';
    int capacity = MAXTEAMNB * MAXTEAM;
    t_myMsgbuf position;

    if (game->game_state != RECRUITMENT || game->team_nbs[team] >= MAXTEAMNB)
        return 1;
    if (recv_msg(player->qid, &position, BROADCAST) != 0)
        return -1;
    if (position.kind != INIT_PLAYER || adjust_player_count(player->sid, 1) == -1) {
        send_msg(player->qid, &position);
        return -1;
    }
    player->x = position.x;
    player->y = position.y;
    game->board[player->x][player->y] = player->team_no;
    player->team_mb_cnt = ++game->team_nbs[team];
    if (++game->player_nbs == capacity) {
        t_myMsgbuf start = {.mytype = START_GAME, .kind = START_GAME,
                            .team_no = NONE, .msg_order = DO_NOTHING};
        for (int i = 0; i < capacity; ++i) {
            if (send_msg(player->qid, &start) == -1) {
                game->game_state = END_GAME;
                game->board[player->x][player->y] = '0';
                --game->player_nbs;
                --game->team_nbs[team];
                adjust_player_count(player->sid, -1);
                return -1;
            }
        }
        game->game_state = START_GAME;
    }
    return 0;
}

int clear_not_reject_player(t_playerData *playerData){
  if (playerData->readonly != NULL ){
    if (detach_board(playerData->readonly) == -1) return (-1);
  }
  if (playerData->readwrite != NULL ){
    if (detach_board(playerData->readwrite) == -1) return (-1);
  }
  return (0);
}
