#include "awale.h"

#include <stdio.h>

int awale_move_is_valid(awale_game *game, int move)
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
    if ((move / (AWALE_BOARD_SIZE / 2)) != game->turn)
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

int awale_play_move(awale_game *game, int move)
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
        ((move / (AWALE_BOARD_SIZE / 2)) != game->turn) &&
        (game->board[holeId] > 1) &&
        (game->board[holeId] < 4))
    {
        game->scores[game->turn] += game->board[holeId];
        game->board[holeId] = 0;
    }

    game->turn++;
    game->turn %= 2;
    // check for end of the game

    return 0;
}

void awale_print_game(awale_game *game)
{
    printf("game:{you:%d,turn:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}\n",
           game->id,
           game->turn,
           game->board[0],
           game->board[1],
           game->board[2],
           game->board[3],
           game->board[4],
           game->board[5],
           game->board[6],
           game->board[7],
           game->board[8],
           game->board[9],
           game->board[10],
           game->board[11],
           game->scores[0],
           game->scores[1]);
    if (game->turn == game->id)
    {
        printf("it's your turn\n");
    }
    else
    {
        printf("it's your opponent's turn\n");
    }
}