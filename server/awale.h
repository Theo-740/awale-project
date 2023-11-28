#ifndef _AWALE_H
#define _AWALE_H

#include "user.h"

#define AWALE_BOARD_SIZE 12
#define AWALE_NB_BEANS_START 4
#define AWALE_MIN_BEANS 3

typedef unsigned char AwaleBoard[AWALE_BOARD_SIZE];

typedef struct AwaleStoredGame
{
    int winner_id;
    int loser_id;
    int scores[2];
    int nbTurns;
    int moves[100];
}AwaleStoredGame;


typedef struct AwaleRunningGame
{
    AwaleBoard board;
    int scores[2];
    int nbTurns;
    int moves[100];
    User* player0;
    User* player1;
    /**
     * -1 if game still running
     * 0 or 1 to designate winner
    */
    int winner;
} AwaleRunningGame;

void awale_init_game(AwaleRunningGame* game);

/**
 * return 0 if move is OK
 * return is negative -> move NOK
 * return -1 if game is already over
 * return -2 if move outside of board
 * return -3 if move on wrong size of the board
 * return -4 if hole is empty
 */
int awale_move_is_valid(AwaleRunningGame* game, int move);

/**
 * update the game to play
 * return is positive or null -> move OK & game updated
 * return 0 if game is still running
 * return 1 if game is over by famine
 * return 2 if game is over by indetermination
 * return is negative -> move NOK & game not changed
 * see awaleMoveIsValid for error code
 */
int awale_play_move(AwaleRunningGame* game, int move);

#endif