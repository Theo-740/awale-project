#ifndef _AWALE_H
#define _AWALE_H

#include "user.h"

#define AWALE_BOARD_SIZE 12
#define AWALE_MIN_BEANS 3
#define AWALE_MAX_TURNS 100

typedef unsigned char AwaleBoard[AWALE_BOARD_SIZE];


typedef struct AwaleGame
{
    int loaded;
    AwaleBoard board;
    int scores[2];
    int id; // my id in the game (am i the zero or first player ?)
    int nbTurns;
    Username opponent;
    /**
     * -1 if game still running
     * 0 or 1 to designate winner
    */
    int winner;
} AwaleGame;

/**
 * return 0 if move is OK
 * return is negative -> move NOK
 * return -1 if game is already over
 * return -2 if move outside of board
 * return -3 if move on wrong size of the board
 * return -4 if hole is empty
 */
int awale_move_is_valid(AwaleGame* game, int move);

/**
 * update the game to play
 * return is positive or null -> move OK & game updated
 * return 0 if game is still running
 * return 1 if game is over by famine
 * return 2 if game is over by indetermination
 * return 3 if game is over because player owns majority of beans
 * return is negative -> move NOK & game not changed
 * see awaleMoveIsValid for error code
 */
int awale_play_move(AwaleGame* game, int move);

void awale_print_game(AwaleGame* game);
void awale_print_game_observe(AwaleGame *game);

#endif