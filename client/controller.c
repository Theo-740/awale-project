#include "controller.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void main_menu_enter(Controller *c);
static void main_menu_user_input(Controller *c, char *message);
static void main_menu_server_input(Controller *c, char *message);

static void game_enter(Controller *c);
static void game_user_input(Controller *c, char *message);
static void game_server_input(Controller *c, char *message);

static void user_list_enter(Controller *c);
static void user_list_user_input(Controller *c, char *message);
static void user_list_server_input(Controller *c, char *message);

static void challenged_enter(Controller *c);
static void challenged_server_input(Controller *c, char *message);
static void challenged_user_input(Controller *c, char *message);

static void challenging_enter(Controller *c);
static void challenging_server_input(Controller *c, char *message);
static void challenging_user_input(Controller *c, char *message);

// main menu state
static void main_menu_enter(Controller *c)
{
    c->state = MAIN_MENU;
    printf("main menu:\n1:Connected users list\n");
}

static void main_menu_user_input(Controller *c, char *message)
{
    int choice;
    if (sscanf(message, "%d", &choice) == 1)
    {
        switch (choice)
        {
        case 1:
            user_list_enter(c);
            break;

        default:
            printf("this number is not an option\n");
            break;
        }
    }
    else
    {
        printf("write a number please\n");
    }
}

static void main_menu_server_input(Controller *c, char *message)
{
    char *header = strtok(message, ":");
    if (!strcmp(header, "challenged"))
    {
        char *username = strtok(NULL, ";");
        strcpy(c->user_list[0], username);
        challenged_enter(c);
        return;
    }
}

// game state
static void game_enter(Controller *c)
{
    c->state = GAME;
    c->game.loaded = 0;
    printf("loading game...\n");
    write_server(c->server_sock, "game_state;");
}

static void game_user_input(Controller *c, char *input)
{

    if (input[0] == 'w')
    {
        write_server(c->server_sock, "withdraw;");
        printf("you withdrew from the game\nyou lost!\n");
        main_menu_enter(c);
        return;
    }
    else if (c->game.turn != c->game.id)
    {
        printf("it's not your turn!!!\n wait for your opponent's move\n");
    }
    else
    {
        int move;
        if (sscanf(input, "%d", &move) != 1 && (move < 0 || move >= AWALE_BOARD_SIZE / 2))
        {
            printf("please enter an integer between 0 and %d\n", AWALE_BOARD_SIZE / 2);
        }
        else
        {
            if (c->game.id == 1)
            {
                move += AWALE_BOARD_SIZE / 2;
            }
            if (awale_play_move(&c->game, move) < 0)
            {
                printf("this move is not valid\n");
            }
            else
            {
                char buffer[BUF_SIZE];
                snprintf(buffer, BUF_SIZE, "move:%d;", move);
                write_server(c->server_sock, buffer);
                awale_print_game(&c->game);
            }
        }
    }
}

static void game_server_input(Controller *c, char *message)
{
    if (!strncmp(message, "game_state:", 11))
    {
        int read = sscanf(
            message,
            "game_state:{you:%d,turn:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}",
            &c->game.id,
            &c->game.turn,
            &c->game.board[0],
            &c->game.board[1],
            &c->game.board[2],
            &c->game.board[3],
            &c->game.board[4],
            &c->game.board[5],
            &c->game.board[6],
            &c->game.board[7],
            &c->game.board[8],
            &c->game.board[9],
            &c->game.board[10],
            &c->game.board[11],
            &c->game.scores[0],
            &c->game.scores[1]);
        if (read == 16)
        {
            c->game.loaded = 1;
            c->game.winner = -1;
            awale_print_game(&c->game);
        }
        else if (read != 0)
        {
            c->game.loaded = 0;
        }
    }
    else if (!strncmp(message, "move:", 5))
    {
        int move;
        if (c->game.turn != c->game.id && sscanf(message, "move:%d", &move) == 1)
        {
            awale_play_move(&c->game, move);
            awale_print_game(&c->game);
        }
        else
        {
            c->game.loaded = 0;
        }
    }
    else if (!strcmp(message, "withdrew"))
    {
        printf("opponent withdrew\nyou won!\n");
        main_menu_enter(c);
        return;
    }
    else if (!strncmp(message, "game_end", 8))
    {
        sscanf(
            message,
            "game_end:{you:%d,winner:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}",
            &c->game.id,
            &c->game.winner,
            &c->game.board[0],
            &c->game.board[1],
            &c->game.board[2],
            &c->game.board[3],
            &c->game.board[4],
            &c->game.board[5],
            &c->game.board[6],
            &c->game.board[7],
            &c->game.board[8],
            &c->game.board[9],
            &c->game.board[10],
            &c->game.board[11],
            &c->game.scores[0],
            &c->game.scores[1]);

        awale_print_game(&c->game);

        main_menu_enter(c);
        return;
    }

    if (!c->game.loaded)
    {
        write_server(c->server_sock, "game state;");
    }
}

// user list state
static void user_list_enter(Controller *c)
{
    c->state = USER_LIST;
    c->nb_users = 0;
    printf("loading list...\n");
    write_server(c->server_sock, "user_list;");
}

static void user_list_user_input(Controller *c, char *input)
{
    if (input[0] == 'q')
    {
        printf("back to main menu\n");
        main_menu_enter(c);
        return;
    }
    if (c->nb_users == 0)
    {
        printf("wait! the list is still loading\n");
        write_server(c->server_sock, "user_list;");
    }
    else
    {
        int id;
        if (sscanf(input, "%d", &id) == 1 && id >= 0 && id < c->nb_users)
        {
            char buffer[BUF_SIZE];
            strncpy(buffer, "challenge:", BUF_SIZE - 1);
            strncat(buffer, c->user_list[id], BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, ";", BUF_SIZE - strlen(buffer) - 1);
            write_server(c->server_sock, buffer);
            strcpy(c->user_list[0], c->user_list[id]);
            challenging_enter(c);
        }
        else
        {
            printf("please enter a valid number\n");
        }
    }
}

static void user_list_server_input(Controller *c, char *message)
{
    char *token = strtok(message, ":");
    if (!strcmp(token, "user_list"))
    {
        token = strtok(NULL, ",");
        while (token != NULL && c->nb_users < MAX_USERS)
        {
            strncpy(c->user_list[c->nb_users], token, USERNAME_LENGTH - 1);
            c->user_list[c->nb_users][USERNAME_LENGTH - 1] = '\0';
            c->nb_users++;
            token = strtok(NULL, ",");
        }
        printf("Connected Users:\n");
        for (int i = 0; i < c->nb_users; i++)
        {
            printf("%d:%s\n", i, c->user_list[i]);
        }
    }
}

// challenged state
static void challenged_enter(Controller *c)
{
    c->state = CHALLENGED;
    printf("%s is challenging you !\n", c->user_list[0]);
    printf("1: accept the challenge\n");
    printf("2: refuse the challenge\n");
}

static void challenged_server_input(Controller *c, char *message)
{
    char *token = strtok(message, ":");
    if (!strcmp(token, "challenge_canceled"))
    {
        printf("challenged was canceled\nback to main menu");
        main_menu_enter(c);
        return;
    }
}

static void challenged_user_input(Controller *c, char *input)
{
    int choice;
    if (sscanf(input, "%d", &choice) == 1)
    {
        switch (choice)
        {
        case 1:
            // user accept the challenge
            printf("you accepted the challenge\n");
            write_server(c->server_sock, "accept_challenge;");
            game_enter(c);
            break;

        case 2:
            // user refuse the challenge
            printf("you refused the challenge\n");
            write_server(c->server_sock, "refuse_challenge;");
            main_menu_enter(c);
            break;

        default:
            printf("this number is not an option\n");
            break;
        }
    }
    else
    {
        printf("write a number please\n");
    }
}

// challenging state
static void challenging_enter(Controller *c)
{
    c->state = CHALLENGING;
    printf("you challenged %s. To undo this action enter anything\n", c->user_list[0]);
}

static void challenging_server_input(Controller *c, char *message)
{
    char *header = strtok(message, ":");
    if (!strcmp(header, "challenge_accepted"))
    {
        printf("%s accepted the challenge\n", c->user_list[0]);
        game_enter(c);
        return;
    }
    else if (!strcmp(header, "challenge_refused"))
    {
        printf("the challenge wad refused\n");
        main_menu_enter(c);
        return;
    }
}

static void challenging_user_input(Controller *c, char *input)
{
    write_server(c->server_sock, "cancel_challenge;");
    printf("you canceled the challenge\nback to main menu\n");
    main_menu_enter(c);
}

// Controller main functions

void controller_init(Controller *c, SOCKET server_sock, const char *username)
{
    char buffer[BUF_SIZE];
    c->server_sock = server_sock;
    /* send our name */
    write_server(server_sock, username);

    /* read server response */
    read_server(server_sock, buffer);

    char *other_messages;
    char *first_message = strtok_r(buffer, ";", &other_messages);
    char *content;
    char *header = strtok_r(first_message, ":", &content);
    if (!strcmp(header, "menu"))
    {
        main_menu_enter(c);
    }
    else if (!strcmp(header, "game"))
    {
        printf("you're currently in a game\n");
        game_enter(c);
    }
    else if (!strcmp(header, "challenged"))
    {
        strcpy(c->user_list[0], content);
        challenged_enter(c);
    }
    else if (!strcmp(header, "challenging"))
    {
        strcpy(c->user_list[0], content);
        challenging_enter(c);
    }
    else if (!strcmp(header, "nope"))
    {
        printf("server refused connection\n");
        c->state = TERMINATED;
        return;
    }
    char *message = strtok(other_messages, ";");
    while (message != NULL)
    {
        controller_server_input(c, message);
        message = strtok(NULL, ";");
    }
}

void controller_user_input(Controller *c, char *message)
{
    switch (c->state)
    {
    case MAIN_MENU:
        main_menu_user_input(c, message);
        break;

    case GAME:
        game_user_input(c, message);
        break;

    case USER_LIST:
        user_list_user_input(c, message);
        break;

    case CHALLENGED:
        challenged_user_input(c, message);
        break;

    case CHALLENGING:
        challenging_user_input(c, message);
        break;

    case TERMINATED:
        break;
    }
};

void controller_server_input(Controller *c, char *message)
{
    if (!strncmp(message, "chat:", 5))
    {
        printf("%s\n", message);
        return;
    }
    if (!strcmp(message, "nope"))
    {
        printf("server disconnected you!\n");
        c->state = TERMINATED;
        return;
    }
    switch (c->state)
    {
    case MAIN_MENU:
        main_menu_server_input(c, message);
        break;

    case GAME:
        game_server_input(c, message);
        break;

    case USER_LIST:
        user_list_server_input(c, message);
        break;

    case CHALLENGED:
        challenged_server_input(c, message);
        break;

    case CHALLENGING:
        challenging_server_input(c, message);
        break;

    case TERMINATED:
        break;
    }
}