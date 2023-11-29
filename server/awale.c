#include "awale.h"

static void compute_winner(AwaleGame *game)
{
    if (game->infos.scores[0] > game->infos.scores[1])
    {
        game->infos.winner = 0;
    }
    else if (game->infos.scores[1] > game->infos.scores[0])
    {
        game->infos.winner = 1;
    }
    else
    {
        game->infos.winner = 2;
    }
}

void awale_init_game(AwaleGame *game)
{
    for (int i = 0; i < AWALE_BOARD_SIZE; i++)
    {
        game->board[i] = AWALE_NB_BEANS_START;
    }
    game->infos.scores[0] = 0;
    game->infos.scores[1] = 0;
    game->infos.nbTurns = 0;
    game->infos.winner = -1;
}

int awale_move_is_valid(AwaleGame *game, int move)
{
    // game already over
    if (game->infos.winner != -1)
    {
        return -1;
    }

    // outside of board
    if (move < 0 || move >= AWALE_BOARD_SIZE)
    {
        return -2;
    }

    // wrong side of the board
    if ((move / (AWALE_BOARD_SIZE / 2)) != game->infos.nbTurns % 2)
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
        ((holeId / (AWALE_BOARD_SIZE / 2)) != game->infos.nbTurns % 2) &&
        (game->board[holeId] > 1) &&
        (game->board[holeId] < 4))
    {
        game->infos.scores[game->infos.nbTurns % 2] += game->board[holeId];
        game->board[holeId] = 0;
        holeId--;
    }

    game->infos.moves[game->infos.nbTurns++] = move;

    // check for end of the game
    int nb_beans[2] = {0, 0};
    for (int i = 0; i < AWALE_BOARD_SIZE; i++)
    {
        nb_beans[i / (AWALE_BOARD_SIZE / 2)] += game->board[i];
    }
    if ((nb_beans[game->infos.nbTurns % 2] == 0) || (nb_beans[0] + nb_beans[1] < AWALE_MIN_BEANS))
    {
        game->infos.scores[0] += nb_beans[0];
        game->infos.scores[1] += nb_beans[1];
        compute_winner(game);
        return 1;
    }
    if (game->infos.scores[(game->infos.nbTurns - 1) % 2] > (game->infos.scores[game->infos.nbTurns % 2] + nb_beans[0] + nb_beans[1]))
    {
        compute_winner(game);
        return 3;
    }
    if (game->infos.nbTurns == AWALE_MAX_TURNS)
    {
        compute_winner(game);
        return 2;
    }

    return 0;
}