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
    printf("opponent: %s\n", game->opponent);
    if (game->id == 0)
    {
        int i = AWALE_BOARD_SIZE - 1;
        printf("%hhd", game->board[i--]);
        while (i >= AWALE_BOARD_SIZE / 2)
        {
            printf("|%hhd", game->board[i--]);
        }
        printf("\n");
        for (i = 0; i < AWALE_BOARD_SIZE / 2 - 1; i++)
        {
            printf("-+");
        }
        printf("-\n");
        i = 0;
        printf("%hhd", game->board[i++]);
        while (i < AWALE_BOARD_SIZE / 2)
        {
            printf("|%hhd", game->board[i++]);
        }
        printf("\n");
    }
    else
    {
        int i = AWALE_BOARD_SIZE / 2 - 1;
        printf("%hhd", game->board[i--]);
        while (i >= 0)
        {
            printf("|%hhd", game->board[i--]);
        }
        printf("\n");
        for (i = 0; i < AWALE_BOARD_SIZE / 2 - 1; i++)
        {
            printf("-+");
        }
        printf("-\n");
        i = AWALE_BOARD_SIZE / 2;
        printf("%hhd", game->board[i++]);
        while (i < AWALE_BOARD_SIZE)
        {
            printf("|%hhd", game->board[i++]);
        }
        printf("\n");
    }
    if (game->winner == -1)
    {
        if (game->turn == game->id)
        {
            for (int i = 0; i < AWALE_BOARD_SIZE / 2; i++)
            {
                printf("^ ");
            }
            printf("\n");
            for (int i = 0; i < AWALE_BOARD_SIZE / 2; i++)
            {
                printf("| ");
            }
            printf("\n");
            for (int i = 0; i < AWALE_BOARD_SIZE / 2; i++)
            {
                printf("%d ", i);
            }
            printf("\n");
            printf("it's your turn\n");
        }
        else
        {
            printf("it's your opponent's turn\n");
        }
    }
    else
    {
        if (game->winner == game->id)
        {
            printf("you won!!\n");
        }
        else
        {
            printf("you lost!!\n");
        }
    }
}