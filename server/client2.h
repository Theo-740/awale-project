#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include "user.h"

typedef struct Client
{
   SOCKET sock;
   int is_observing;
   User *user;
} Client;

#endif /* guard */
