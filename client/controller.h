#ifndef STATE_H
#define STATE_H

#include "client2.h"
#include "awale.h"
#include "user.h"

#define MAX_USERS 16

typedef enum ControllerState
{
    TERMINATED,
    MAIN_MENU,
    GAME,
    USER_LIST,
    CHALLENGED,
    CHALLENGING,
} ControllerState;

typedef struct Controller
{
    ControllerState state;
    SOCKET server_sock;
    awale_game game;
    int nb_users;
    Username user_list[MAX_USERS];
} Controller;

void controller_init(Controller *c, SOCKET server_sock, const char* username);
void controller_user_input(Controller *c, char *input);
void controller_server_input(Controller *c, char *message);

#endif