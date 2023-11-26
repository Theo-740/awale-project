#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if (err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   int max_fd = sock;
   char buffer[BUF_SIZE];

   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   int nb_clients = 0;

   User users[30];
   int nb_users = 0;

   fd_set rdfs;

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for (i = 0; i < nb_clients; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max_fd + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = {0};
         socklen_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if (read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         int user_id = connect_user(users, &nb_users, buffer);
         if (user_id < 0)
         {
            write_client(csock, "nope");
            closesocket(csock);
         }
         else
         {
            /* what is the new maximum fd ? */
            max_fd = csock > max_fd ? csock : max_fd;

            FD_SET(csock, &rdfs);

            // strncpy(c.name, buffer, BUF_SIZE - 1);
            Client *c = &clients[nb_clients];
            nb_clients++;
            c->sock = csock;
            c->user_id = user_id;

            if (users[user_id].is_playing == 1)
            {
               c->state = PLAYING;
               write_client(csock, "game");
            }
            else
            {
               c->state = MENU;
               write_client(csock, "menu");
            }
            strncpy(buffer, users[user_id].name, BUF_SIZE - 1);
            strncat(buffer, " connected !", BUF_SIZE - strlen(buffer) - 1);
            send_message_to_all_clients(clients, *c, nb_clients, users, buffer, 1);
         }
      }
      else
      {
         int i = 0;
         for (i = 0; i < nb_clients; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &nb_clients, users);
                  strncpy(buffer, users[client.user_id].name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, nb_clients, users, buffer, 1);
                  
               }
               else
               {
                  char *token = strtok(buffer, ":");
                  if (!strcmp(token, "chat"))
                  {
                     send_message_to_all_clients(clients, client, nb_clients, users, buffer + 5, 0);
                  }
                  /* show player list */
                  else if (!strcmp(token, "user_list"))
                  {
                     send_user_list_to_client(client, clients, nb_clients, users);
                  }
                  /* challenge a user if didn't already asked someone */
                  else if (!strcmp(token, "challenge") )
                  {
                     token = strtok(NULL, "\n");
                     int challenged_id = find_user(users, nb_users, token);
                     if (challenged_id != -1 && users[challenged_id].is_connected && !users[challenged_id].is_playing && clients[find_client(clients, nb_clients, challenged_id)].state != WAITING_ANSWER && client.state != WAITING_ANSWER)
                     {
                        Client challenged = clients[find_client(clients, nb_clients, challenged_id)];
                        printf("challenged state : %d\n", challenged.state);
                        strncpy(buffer, "challenged:", BUF_SIZE - 1);
                        strncat(buffer, users[client.user_id].name, BUF_SIZE - strlen(buffer) - 1);
                        write_client(challenged.sock, buffer);
                        // TODO améliorer ça après discution mais pour l'instant relation 1/1
                        clients[find_client(clients, nb_clients, challenged_id)].state = WAITING_ANSWER;
                        clients[i].state = WAITING_ANSWER;

                     }
                     else
                     {
                        write_client(client.sock, "challenge:fail");
                     }
                  }
                  /* accept a challenge */
                  else if(!strcmp(token, "game_accepted"))
                  {
                     token = strtok(NULL, "\n");                     
                     int opponent_id = find_user(users, nb_users, token);

                     users[client.user_id].is_playing = 1;
                     users[opponent_id].is_playing = 1;

                     ClientState statePlayer1 = (rand()>0.5)? WAITING_MOVE:PLAYING;
                     clients[i].state = statePlayer1;
                     clients[find_client(clients, nb_clients, opponent_id)].state = (statePlayer1 == WAITING_MOVE)? PLAYING:WAITING_MOVE;
                  }
                  /*refused challenge*/
                  else if(!strcmp(token, "game_refused"))
                  {
                     token = strtok(NULL, "\n");                     
                     int refused_id = find_user(users, nb_users, token);

                     clients[i].state = MENU;
                     clients[find_client(clients, nb_clients, refused_id)].state = MENU;
                     write_client(clients[find_client(clients, nb_clients, refused_id)].sock,"game_refused");
                  }
               }
               break;
            }
         }
      }
   }
   printf("shuting down\n");
   clear_clients(clients, nb_clients);
   end_connection(sock);
}

static int find_client(Client *clients, int nb_clients, int user_id)
{
   for (int i = 0; i < nb_clients; i++)
   {
      if (clients[i].user_id == user_id)
      {
         return i;
      }
   }
   return -1;
}

static int find_user(User *users, int nb_users, char *username)
{
   for (int id = 0; id < nb_users; ++id)
   {
      if (!strcmp(username, users[id].name))
      {
         return id;
      }
   }
   return -1;
}

static int connect_user(User *users, int *nb_users, char *username)
{
   if (strlen(username) >= USERNAME_LENGTH || strchr(username, ',') != NULL || strchr(username, ':') != NULL)
   {
      return -1;
   }
   int id = find_user(users, *nb_users, username);
   if (id == -1)
   {
      id = *nb_users;
      strcpy(users[id].name, username);
      users[id].is_connected = 1;
      users[id].is_playing = 0;
      (*nb_users)++;
   }
   else
   {
      if (users[id].is_connected == 1)
      {
         return -1;
      }
      users[id].is_connected = 1;
   }
   return id;
}

static void clear_clients(Client *clients, int nb_clients)
{
   int i = 0;
   for (i = 0; i < nb_clients; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *nb_clients, User *users)
{
   users[clients[to_remove].user_id].is_connected = 0;
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*nb_clients - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*nb_clients)--;
}

static void send_message_to_all_clients(Client *clients, Client source, int nb_clients, User *users, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   strncpy(message, "chat:", BUF_SIZE - 1);
   for (i = 0; i < nb_clients; i++)
   {
      /* we don't send message to the source */
      if (source.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncat(message, users[source.user_id].name, BUF_SIZE - 1);
            strncat(message, ": ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static void send_user_list_to_client(Client target, Client *clients, int nb_clients, User *users)
{
   int i = 0;
   char message[BUF_SIZE];
   strncpy(message, "user_list:", BUF_SIZE - 1);
   for (i = 0; i < nb_clients; i++)
   {
      /* we don't send his name to the target */
      if (target.sock != clients[i].sock)
      {
         strncat(message, users[clients[i].user_id].name, sizeof message - strlen(message) - 1);
         strncat(message, ",", sizeof message - strlen(message) - 1);
      }
   }
   write_client(target.sock, message);
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   printf("sock n°%d says : \"%s\"\n", sock, buffer);

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }

   printf("I say : \"%s\"\n", buffer);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
