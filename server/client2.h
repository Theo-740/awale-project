#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"
#include "user.h"


typedef struct Client
{
   SOCKET sock;
   User *user;
} Client;

#endif /* guard */
