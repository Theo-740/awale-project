#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>
#include <windows.h>

// #elif defined (linux)
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

// #else

// #error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define MAX_USERS 100
#define MAX_STORED_GAMES 200
#define MAX_PLAYING_GAMES 50
#define MAX_OBSERVERS 10

#define BUF_SIZE 1024
#define MAX_MESSAGES 16

#include "client2.h"
#include "awale.h"
#include "user.h"

typedef struct RunningGame
{
    int id;
    User *player0;
    User *player1;
    int nb_observers;
    User *observers[MAX_OBSERVERS];
    AwaleGame awale;
} RunningGame;

/**
 * A game that is over 
*/
typedef struct StoredGame
{
    int id;
    User *player0;
    User *player1;
    AwaleGameInfos awale;
} StoredGame;

static void init(void);
static void end(void);
static void app(void);

static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);

static void connect_client(SOCKET sock, int *max_fd, fd_set *rdfs);
static void disconnect_client(Client *client);

/**
 * Check if the conditions for challenging the user are right
 * if so, send a challenge message to the challenged user
*/
static void challenge_user(Client *challenger, const char *username);
/**
 * Accept the challenge and start a game
 * Notify the challenging player
*/
static void accept_challenge(Client *client);
/**
 * Refuse the challenge
 * Notify the challenging player
*/
static void refuse_challenge(Client *client);
/**
 * Cancel the challenge
 * Notify the challenger
*/
static void cancel_challenge(Client *client);

/**
 * Send the state of a running game
*/
static void send_game(Client *client, RunningGame *game);
/**
 * Check if the move is valid
 * If so play the move and notify the other player and observers
*/
static void make_move(Client *client, const char *move_description);

static RunningGame *find_running_game_by_player(User *user);
static RunningGame *find_running_game_by_observer(Client *client);
static RunningGame *find_running_game_by_id(int id);
/**
 * Store the game in the stored games list
 * Removes it from the running games list
*/
static StoredGame *store_game(RunningGame *game);

static Client *find_client(const User *user);
static User *find_user(const char *username);
/**
 * Check if the user name is valid 
 * if the user exists and is not connected, connect the user
 * else create a new user
*/
static User *connect_user(const char *username);

/**
 * Add a observer to a game
*/
static void add_observer(RunningGame *game, Client *observer);
/**
 * Remove observer from a game
*/
static void remove_observer(RunningGame *game, Client *observer);

static void clear_clients();

static void remove_client(Client *client);

static void send_chat_message_to_all_clients(Client *source, const char *buffer, char from_server);
static void send_user_list_to_client(Client *target);
static void send_list_running_games(Client *target);

#endif /* guard */
