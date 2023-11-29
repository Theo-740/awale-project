#include "awale.h"

#include <stdio.h>

static void compute_winner(AwaleGame *game)
{
    if (game->scores[0] > game->scores[1])
    {
        game->winner = 0;
    }
    else if (game->scores[1] > game->scores[0])
    {
        game->winner = 1;
    }
    else
    {
        game->winner = 2;
    }
}

int awale_move_is_valid(AwaleGame *game, int move)
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

int awale_play_move(AwaleGame *game, int move)
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
        ((holeId / (AWALE_BOARD_SIZE / 2)) != game->nbTurns % 2) &&
        (game->board[holeId] > 1) &&
        (game->board[holeId] < 4))
    {
        game->scores[game->nbTurns % 2] += game->board[holeId];
        game->board[holeId] = 0;
        holeId--;
    }

    game->nbTurns++;
    // check for end of the game
    int nb_beans[2] = {0, 0};
    for (int i = 0; i < AWALE_BOARD_SIZE; i++)
    {
        nb_beans[i / (AWALE_BOARD_SIZE / 2)] += game->board[i];
    }
    if ((nb_beans[game->nbTurns % 2] == 0) || (nb_beans[0] + nb_beans[1] < AWALE_MIN_BEANS))
    {
        game->scores[0] += nb_beans[0];
        game->scores[1] += nb_beans[1];
        compute_winner(game);
        return 1;
    }
    if (game->scores[(game->nbTurns - 1) % 2] > (game->scores[game->nbTurns % 2] + nb_beans[0] + nb_beans[1]))
    {
        compute_winner(game);
        return 3;
    }
    if (game->nbTurns == AWALE_MAX_TURNS)
    {
        compute_winner(game);
        return 2;
    }

    return 0;
}

void awale_print_game(AwaleGame *game)
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
        printf("     score:%d\n", game->scores[1]);
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
        printf("     score:%d\n", game->scores[0]);
    }
    else
    {
        int i = AWALE_BOARD_SIZE / 2 - 1;
        printf("%hhd", game->board[i--]);
        while (i >= 0)
        {
            printf("|%hhd", game->board[i--]);
        }
        printf("     score:%d\n", game->scores[1]);
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
        printf("     score:%d\n", game->scores[0]);
    }
    if (game->winner == -1)
    {
        if (game->nbTurns % 2 == game->id)
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
        else if (game->winner == 2)
        {
            printf("it's a draw!!\n");
        }
        else
        {
            printf("you lost!!\n");
        }
    }
}