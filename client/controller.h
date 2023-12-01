#ifndef STATE_H
#define STATE_H

#include "client2.h"
#include "awale.h"
#include "user.h"

#define MAX_USERS 16
#define MAX_GAMES 16

/**
 * Controller inspired by the design pattern controller 
 * Handle server and user input and console output 
 * Inputs are given by the main program
*/

typedef struct Game
{
    Username player0;
    Username player1;
    int self_id;
    AwaleGame awale;
} Game;

typedef enum ControllerState
{
    TERMINATED,
    MAIN_MENU,
    GAME,
    USER_LIST,
    CHALLENGED,
    CHALLENGING,
    GAME_LIST,
    OBSERVER,
} ControllerState;

typedef struct Controller
{
    ControllerState state;
    SOCKET server_sock;
    Username name;

    int loaded;
    
    Game game;
    int nb_games;
    int game_list_id[MAX_GAMES];
    Username game_list_name[2*MAX_GAMES];
    int nb_users;
    Username user_list[MAX_USERS];
    int state_list[MAX_USERS];
} Controller;

/**
 * Initialize the controller 
*/
void controller_init(Controller *c, SOCKET server_sock, const char *username);

/**
 * Handle user input
*/
void controller_user_input(Controller *c, char *input);

/**
 * Handle server input
*/
void controller_server_input(Controller *c, char *message);

#endif