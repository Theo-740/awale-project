#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

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

#define BUF_SIZE 1024

#include "client2.h"
#include "awale.h"
#include "user.h"

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client sender, int nb_clients, User *users, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *nb_clients, User *users);
static void clear_clients(Client *clients, int actual);
static void print_all_users(User* users, int nb_user);
static void send_user_list_to_client(Client target, Client *clients, int nb_clients, User *users);
static User* connect_user(User *users, int *nb_users, char *username);
static Client* find_client(Client *clients, int nb_clients, User* user);
static User* find_user(User *users, int nb_users, char *username);
static AwaleRunningGame *find_awale_running(User *user, AwaleRunningGame *awale_running, int nb_awale_running);

#endif /* guard */
