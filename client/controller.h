#ifndef STATE_H
#define STATE_H

#include "client2.h"
#include "awale.h"
#include "user.h"

#define MAX_USERS 16

typedef enum ControllerState
{
    MAIN_MENU,
    GAME,
    USER_LIST,
    CHALLENGED,
} ControllerState;

typedef struct Controller
{
    ControllerState state;
    awale_game game;
    int nb_users;
    Username user_list[MAX_USERS];
    int is_challenged;
    Username challenger;
} Controller;

void controller_init(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE], ControllerState state);
void controller_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
void controller_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);

static void main_menu_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void main_menu_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void main_menu_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);

static void game_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void game_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void game_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);

static void user_list_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void user_list_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void user_list_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);

static void challenged_enter(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void new_challenge_server_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void new_challenge_user_input(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);
static void reshow_infos_challenge(Controller *c, SOCKET serv_sock, char buffer[BUF_SIZE]);

#endif