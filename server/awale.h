#ifndef _AWALE_H
#define _AWALE_H

#include "user.h"

#define AWALE_BOARD_SIZE 12
#define AWALE_NB_BEANS_START 4

typedef unsigned char awale_board_t[AWALE_BOARD_SIZE];

typedef struct awale_stored_game_t
{
    int winner_id;
    int loser_id;
    int scores[2];
    int nbTurns;
    int moves[100];
}awale_stored_game_t;


typedef struct awale_running_game_t
{
    awale_board_t board;
    int scores[2];
    int nbTurns;
    int moves[100];
    int player0_id;
    int player1_id;
    /**
     * -1 if game still running
     * 0 or 1 to designate winner
    */
    int winner;
} awale_running_game_t;

void awale_init_game(awale_running_game_t* game);

/**
 * return 0 if move is OK
 * return is negative -> move NOK
 * return -1 if game is already over
 * return -2 if move outside of board
 * return -3 if move on wrong size of the board
 * return -4 if hole is empty
 */
int awale_move_is_valid(awale_running_game_t* game, int move);

/**
 * update the game to play
 * return is positive or null -> move OK & game updated
 * return 0 if game is still running
 * return 1 if game is over by famine
 * return 2 if game is over by indetermination
 * return is negative -> move NOK & game not changed
 * see awaleMoveIsValid for error code
 */
int awale_play_move(awale_running_game_t* game, int move);

#endif