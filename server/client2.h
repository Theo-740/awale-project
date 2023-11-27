#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include "user.h"

typedef enum ClientState
{
   MENU,           // in the menu nothing special happening
   WAITING_ANSWER, // waiting for answer for a game
   PLAYING,        // playing a move
   WAITING_MOVE,   // waiting for opponent to play a move
   OBSERVING       // observing a game
} ClientState;

typedef struct Client
{
   SOCKET sock;
   User *user;
   ClientState state;
} Client;

#endif /* guard */
