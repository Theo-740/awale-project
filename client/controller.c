#include "controller.h"

#include <stdio.h>
#include <string.h>

// main menu state
static void main_menu_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    c->state = MAIN_MENU;
    printf("main menu:\n1:Connected users list\n");
}

static void main_menu_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    int choice;
    if (sscanf(buffer, "%d", &choice) == 1)
    {
        switch (choice)
        {
        case 1:
            user_list_enter(c, serv_sock, buffer);
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

static void main_menu_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    printf("I cannot read what the server sends me\n");
}

static void challenged_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    c->state = CHALLENGED;
}

static void reshow_infos_challenge(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    char *token = strtok(buffer, ":");
    if(!strcmp(token, "challenged"))
    {
        new_challenge_server_input(c,serv_sock,buffer);
    } else
    {
        token = strtok(NULL, ",");
        strcpy(c->challenger, token);
        printf("you challenged %s. To undo this action enter anything\n", token);
    }
}

static void new_challenge_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    char *token = strtok(buffer, ":");
    if (!strcmp(token, "challenged"))
    {
        c->is_challenged = 1;
        
        challenged_enter(c,serv_sock,buffer);
        token = strtok(NULL, ",");
        strcpy(c->challenger, token);
        
        printf("%s wants to challenge you !\n", c->challenger);
        printf("1: accept the challenge\n");
        printf("2: refuse the challenge\n");
    }
}

static void new_challenge_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    if (c->is_challenged == 1)
    {
        int choice;
        if (sscanf(buffer, "%d", &choice) == 1)
        {
            switch (choice)
            {
            case 1:
            // user accept the challenge
                c->is_challenged = 0;
                strncpy(buffer, "game_accepted:", BUF_SIZE-1);
                strncat(buffer, c->challenger, BUF_SIZE -strlen(buffer)-1);
                write_server(serv_sock, buffer);
                game_enter(c,serv_sock,buffer);
                break;

            case 2:
            // user refuse the challenge
                c->is_challenged = 0;
                printf("went to refuse %d\n", c->state);
                strncpy(buffer, "game_refused:", BUF_SIZE-1);
                strncat(buffer, c->challenger, BUF_SIZE -strlen(buffer)-1);
                write_server(serv_sock, buffer);
                main_menu_enter(c,serv_sock,buffer);
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
    }else{
        strncpy(buffer, "game_refused:", BUF_SIZE-1);
        strncat(buffer, c->challenger, BUF_SIZE -strlen(buffer)-1);
        write_server(serv_sock, buffer);

        main_menu_enter(c,serv_sock, buffer);
    }
}

// game state
static void game_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    c->state = GAME;
    c->game.loaded = 0;
    printf("loading game...\n");
}

static void game_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{

    if (buffer[0] == 'w')
    {
        write_server(serv_sock, "withdrawal");
        printf("you withdrew from the game\nyou lost!\n");
        main_menu_enter(c, serv_sock, buffer);
        return;
    }
    else if (c->game.turn != c->game.id)
    {
        printf("it's not your turn!!!\n wait for your opponent's move\n");
    }
    else
    {
        int move;
        if (sscanf(buffer, "%d", &move) != 1 || awale_play_move(&c->game, move) < 0)
        {
            printf("this move is not valid\n");
        }
        else
        {
            snprintf(buffer, BUF_SIZE, "move:%d\n", move);
            write_server(serv_sock, buffer);
        }
    }
}

static void game_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    if (!strncmp(buffer, "game_state:", 11))
    {
        int read = sscanf(
            buffer,
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
            awale_print_game(&c->game);
        }
        else if (read != 0)
        {
            c->game.loaded = 0;
        }
    }
    else if (!strncmp(buffer, "move:", 5))
    {
        int move;
        if (c->game.turn != c->game.id && sscanf(buffer, "move:%d", &move) == 1)
        {
            awale_play_move(&c->game, move);
            awale_print_game(&c->game);
        }
        else
        {
            c->game.loaded = 0;
        }
    }
    else if (!strncmp(buffer, "withdrawal", 10))
    {
        printf("opponent withdrew\nyou won!\n");
        main_menu_enter(c, serv_sock, buffer);
        return;
    }

    if (!c->game.loaded)
    {
        write_server(serv_sock, "game state");
    }
}

// user list state
static void user_list_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    c->state = USER_LIST;
    c->nb_users = 0;
    printf("loading list...\n");
    write_server(serv_sock, "user_list");
}

static void user_list_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    if (c->nb_users == 0)
    {
        printf("wait! the list is still loading\n");
        write_server(serv_sock, "user_list");
    } else {
        int id;
        if(sscanf(buffer,"%d",&id) == 1 && id >= 0 && id < c->nb_users) {
            strncpy(buffer, "challenge:", BUF_SIZE-1);
            strncat(buffer, c->user_list[id], BUF_SIZE -strlen(buffer)-1);
            write_server(serv_sock, buffer);
            strcpy(c->challenger,c->user_list[id]);

            printf("you challenged %s. To undo this action enter anything\n", c->user_list[id]);
            challenged_enter(c,serv_sock,buffer);
        } else {
            printf("please enter a valid number\n");
        }
    }
}

static void user_list_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    char *token = strtok(buffer, ":");
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

// Controller main functions

void controller_init(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE], ControllerState state)
{
    switch (state)
    {
    case MAIN_MENU:
        main_menu_enter(c, serv_sock, buffer);
        break;

    case GAME:
        printf("you are currently in a game\n");
        game_enter(c, serv_sock, buffer);
        break;
    case CHALLENGED:
        challenged_enter(c, serv_sock, buffer);
        reshow_infos_challenge(c,serv_sock, buffer);
        break;
    }
}

void controller_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    switch (c->state)
    {
    case MAIN_MENU:
        main_menu_user_input(c, serv_sock, buffer);
        break;

    case GAME:
        game_user_input(c, serv_sock, buffer);
        break;

    case USER_LIST:
        user_list_user_input(c, serv_sock, buffer);
        break;

    case CHALLENGED:
        new_challenge_user_input(c, serv_sock, buffer);
        break;
    }
};

void controller_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE])
{
    printf("state : %d\n", c->state);
    if(!strncmp(buffer, "chat:",5)){
        printf("%s\n",buffer);
        return;
    }
    switch (c->state)
    {
    case MAIN_MENU:
        printf("went to controller server input main menu\n");
        if(!strncmp(buffer,"challenged:",11)){
            new_challenge_server_input(c,serv_sock, buffer);
        }
        else{
        main_menu_server_input(c, serv_sock, buffer);
        }
        break;

    case GAME:
        printf("went to controller server input game\n");
        game_server_input(c, serv_sock, buffer);
        break;

    case USER_LIST:
        printf("went to controller server input main user list\n");
        user_list_server_input(c, serv_sock, buffer);
        break;

    case CHALLENGED:
        if(!strncmp(buffer,"game_refused",12)){
            printf("went to controller server input challenge refused\n");
            main_menu_enter(c,serv_sock,buffer);
        }
        else if(!strncmp(buffer,"game_accepted",13)){
            printf("went to controller server input challenge accepted\n");
            game_enter(c,serv_sock,buffer);
        }
        break;

    default :
        printf("went to controller server input default\n");
        break;
    }
}