#include "controller.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void read_awale_game(Controller *c, char *string)
{
    c->game.loaded = 0;

    Username player0;
    Username player1;

    char *token;
    if ((token = strtok(string, ",")) == NULL)
        return;
    sscanf(token, "%s", player0);
    if ((token = strtok(NULL, ",")) == NULL)
        return;
    sscanf(token, "%s", player1);

    if (!strcmp(player0, c->name))
    {
        c->game.id = 0;
        strcpy(c->game.opponent, player1);
    }
    else if (!strcmp(player1, c->name))
    {
        c->game.id = 1;
        strcpy(c->game.opponent, player0);
    }
    else
    {
        return;
    }

    if ((token = strtok(NULL, ",")) == NULL)
        return;
    sscanf(token, "%d", &c->game.scores[0]);
    if ((token = strtok(NULL, ",")) == NULL)
        return;
    sscanf(token, "%d", &c->game.scores[1]);
    if ((token = strtok(NULL, ",")) == NULL)
        return;
    sscanf(token, "%d", &c->game.nbTurns);

    for (int i = 0; i < AWALE_BOARD_SIZE; i++)
    {
        if ((token = strtok(NULL, ",")) == NULL)
            return;
        sscanf(token, "%hhd", &c->game.board[i]);
    }

    c->game.winner = -1;

    c->game.loaded = 1;
}

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

static void games_list_enter(Controller *c);
static void games_list_server_input(Controller *c, char *message);
static void games_list_user_input(Controller *c, char *message);

// main menu state
static void main_menu_enter(Controller *c)
{
    c->state = MAIN_MENU;
    printf("main menu:\n1:Connected users list\n2:Games running \n");
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

        case 2:
            games_list_enter(c);
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
    else if (c->game.nbTurns % 2 != c->game.id)
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
    char *content;
    char *header = strtok_r(message, ":", &content);
    if (!strcmp(header, "game_state"))
    {
        read_awale_game(c, content);
        if(c->game.loaded){
            awale_print_game(&c->game);
        }
    }
    else if (strcmp(header, "move"))
    {
        int move;
        if (c->game.nbTurns % 2 != c->game.id && sscanf(content, "%d", &move) == 1)
        {
            awale_play_move(&c->game, move);
            awale_print_game(&c->game);
        }
        else
        {
            c->game.loaded = 0;
        }
    }
    else if (!strcmp(header, "withdrew"))
    {
        printf("opponent withdrew\nyou won!\n");
        main_menu_enter(c);
        return;
    }
    else if (!strncmp(header, "game_end", 8))
    {
        sscanf(
            content,
            "{you:%d,winner:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}",
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

// games list state
static void games_list_enter(Controller *c)
{
    c->state = GAME_LIST;
    c->nb_users = 0;
    printf("loading list...\n");
    write_server(c->server_sock, "running_games_list;");
}

static int member(int *element, Controller *c)
{
    for(int i=0; i<(c->nb_games/2);++i)
    {
        if((*element) == c->games_list_id[i])
        {
            return 1;
        }
    }
    return 0;
}
static void games_list_user_input(Controller *c, char *input)
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
        write_server(c->server_sock, "running_games_list;");
    }
    else
    {
        int id;
        if (sscanf(input, "%d", &id) == 1 && member(&id,c))
        {
            char buffer[BUF_SIZE];
            strncpy(buffer, "running_games_list:", BUF_SIZE - 1);
            strncat(buffer, id, BUF_SIZE - strlen(buffer) - 1);
            strncat(buffer, ";", BUF_SIZE - strlen(buffer) - 1);
            write_server(c->server_sock, buffer);
            //observer_enter(c);
        }
        else
        {
            printf("please enter a valid number\n");
        }
    }
}

static void games_list_server_input(Controller *c, char *message)
{
    char *header = strtok(message, ":");
    if (!strcmp(header, "running_games_list"))
    {
        char *game = strtok(NULL, ":");
        while (game != NULL && c->nb_games < MAX_USERS)
        {
            c->games_list_id[c->nb_games/2] = atoi(message);
            //sscanf(message, "%d", &c->games_list_id[c->nb_games/2]);
            game = strtok(NULL,"-");
            strcpy(c->games_list_name[c->nb_games], game, USERNAME_LENGTH - 1);
            c->games_list_name[c->nb_games][USERNAME_LENGTH - 1] = '\0';
            c->nb_users++;
            game = strtok(NULL, ",");
            strncpy(c->games_list_name[c->nb_games], game, USERNAME_LENGTH - 1);
            c->games_list_name[c->nb_games][USERNAME_LENGTH - 1] = '\0';
            c->nb_users++;
            game = strtok(NULL, ":");
        }
        printf("Games Running:\n Insert a number to observe the corresponding game \n");
        for (int i = 0; i < c->nb_users; i= i+2)
        {
            printf("%d:%s-%s\n", c->games_list_id[i], c->games_list_name[i], c->games_list_name[i+1]);
        }
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
    char *header = strtok(message, ":");
    if (!strcmp(header, "user_list"))
    {
        char *user = strtok(NULL, ",");
        while (user != NULL && c->nb_users < MAX_USERS)
        {
            strncpy(c->user_list[c->nb_users], user, USERNAME_LENGTH - 1);
            c->user_list[c->nb_users][USERNAME_LENGTH - 1] = '\0';
            c->nb_users++;
            user = strtok(NULL, ",");
        }
        printf("Connected Users:\n Insert a number to challenge the corresponding user \n");
        for (int i = 0; i < c->nb_users; i++)
        {
            printf("%d:%s\n", i, c->user_list[i]);
        }
    }
    else if (!strcmp(header, "challenged"))
    {
        char *username = strtok(NULL, ";");
        strcpy(c->user_list[0], username);
        challenged_enter(c);
        return;
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
    else if (!strcmp(header, "challenged"))
    {
        printf("the challenge wad refused\n");
        char *username = strtok(NULL, ";");
        strcpy(c->user_list[0], username);
        challenged_enter(c);
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
    strcpy(c->name, username);
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
    
    case GAME_LIST:
        games_list_user_input(c, message);
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

    case GAME_LIST:
        games_list_server_input(c, message);
        break;

    case TERMINATED:
        break;
    }
}