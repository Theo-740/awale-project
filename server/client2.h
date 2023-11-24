#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

enum client_state
{
   MENU,           // in the menu nothing special happening
   WAITING_ANSWER, // waiting for answer for a game
   PLAYING,        // playing a move
   WAITING_MOVE,   // waiting for opponent to play a move
   OBSERVING       // observing a game
};

typedef struct
{
   SOCKET sock;
   int user_id; // user id in the user list
} Client;

#endif /* guard */
