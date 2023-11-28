#include "awale.h"

void awale_init_game(AwaleRunningGame *game)
{
    for (int i = 0; i < AWALE_BOARD_SIZE; i++)
    {
        game->board[i] = AWALE_NB_BEANS_START;
    }
    game->scores[0] = 0;
    game->scores[1] = 0;
    game->nbTurns = 0;
    game->winner = -1;
}

int awale_move_is_valid(AwaleRunningGame *game, int move)
{
    // game already over
    if (game->winner != -1)
    {
        return -1;
    }

    // outside of board
    if (move < 0 || move >= AWALE_BOARD_SIZE)
    {
        return -2;
    }

    // wrong side of the board
    if ((move / (AWALE_BOARD_SIZE / 2)) != game->nbTurns % 2)
    {
        return -3;
    }

    // no beans in the hole
    if (game->board[move] == 0)
    {
        return -4;
    }

    return 0;
}

int awale_play_move(AwaleRunningGame *game, int move)
{
    int basicValidity = awale_move_is_valid(game, move);
    if (basicValidity < 0)
    {
        return basicValidity;
    }

    // moves the beans
    int holeId = move;
    do
    {
        holeId++;
        holeId %= AWALE_BOARD_SIZE;
        if (holeId != move)
        {
            game->board[holeId]++;
            game->board[move]--;
        }
    } while (game->board[move] > 0);

    // captures the beans
    while (
        ((move / (AWALE_BOARD_SIZE / 2)) != game->nbTurns % 2) &&
        (game->board[holeId] > 1) &&
        (game->board[holeId] < 4))
    {
        game->scores[game->nbTurns % 2] += game->board[holeId];
        game->board[holeId] = 0;
    }

    game->moves[game->nbTurns++] = move;

    // check for end of the game
    int nb_beans = 0;
    for( int i = 0; i<AWALE_BOARD_SIZE/2; i++) {
        nb_beans += game->board[i+(game->nbTurns%2)*AWALE_BOARD_SIZE/2];
    }
    if(nb_beans == 0) {
        game->winner = (game->nbTurns+1)%2;
        return 1;
    }

    return 0;
}