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

   AwaleRunningGame awale_running[MAX_CLIENTS];
   int nb_awale_running = 0;

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

         User *user = connect_user(users, &nb_users, buffer);
         if (user == NULL)
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
            c->user = user;

            if (c->user->is_playing == 1)
            {
               c->state = PLAYING;
               write_client(csock, "game");
            }
            else if (c->user->is_challenging == 1)
            {
               strcpy(buffer, "challenge:");
               strcat(buffer, c->user->challenged);
               write_client(csock, buffer);
            }
            else if (c->user->is_challenged == 1)
            {
               strcpy(buffer, "challenge:");
               strcat(buffer, c->user->challenged);
               write_client(csock, buffer);
            }
            else
            {
               c->state = MENU;
               write_client(csock, "menu");
            }
            strncpy(buffer, c->user->name, BUF_SIZE - 1);
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
                  strncpy(buffer, client.user->name, BUF_SIZE - 1);
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
                  else if (!strcmp(token, "challenge") && client.user->is_challenged != 1)
                  {
                     token = strtok(NULL, "\n");
                     User *challenged_user = find_user(users, nb_users, token);
                     if (challenged_user != NULL && challenged_user->is_connected && !challenged_user->is_playing && challenged_user->is_challenged != 1)
                     {
                        Client *challenged_client = find_client(clients, nb_clients, challenged_user);
                        printf("challenged state : %d\n", challenged_client->state);
                        strncpy(buffer, "challenged:", BUF_SIZE - 1);
                        strncat(buffer, client.user->name, BUF_SIZE - strlen(buffer) - 1);
                        write_client(challenged_client->sock, buffer);
                        // TODO améliorer ça après discution mais pour l'instant relation 1/1
                        client.user->is_challenged = 1;
                        client.user->is_challenging = 1;
                        challenged_user->is_challenged = 1;

                        strcpy(client.user->challenged, challenged_user->name);
                        strcpy(client.user->challenged, client.user->name);
                     }
                     else
                     {
                        write_client(client.sock, "game_refused");
                     }
                  }
                  /* accept a challenge */
                  else if (client.user->is_challenged == 1 && !strcmp(token, "game_accepted"))
                  {
                     token = strtok(NULL, "\n");
                     User *opponent_user = find_user(users, nb_users, client.user->challenged);
                     Client* opponent_client = find_client(clients, nb_clients, opponent_user);
                     write_client(opponent_client->sock, "game_accepted");

                     client.user->is_playing = 1;
                     opponent_user->is_playing = 1;

                     client.user->is_challenged = 0;
                     client.user->is_challenging = 0;
                     opponent_user->is_challenged = 0;

                     // choose player one
                     ClientState statePlayer1 = (rand() > 0.5) ? WAITING_MOVE : PLAYING;
                     clients[i].state = statePlayer1;
                     opponent_client->state = (statePlayer1 == WAITING_MOVE) ? PLAYING : WAITING_MOVE;

                     // create new awale running
                     awale_init_game(&awale_running[nb_awale_running]);
                     awale_running[nb_awale_running].player0 = (statePlayer1 == WAITING_MOVE) ? opponent_user : clients[i].user;
                     awale_running[nb_awale_running].player1 = (statePlayer1 == WAITING_MOVE) ? clients[i].user : opponent_user;
                     ++nb_awale_running;
                     printf("new awale created !\n");
                     // TODO send message with info about the game (don't know what theo did/didn't do)
                  }
                  /*refused challenge*/
                  else if (client.user->is_challenged == 1 && !strcmp(token, "game_refused"))
                  {
                     token = strtok(NULL, "\n");
                     User *challenger = find_user(users, nb_users, token);

                     client.user->is_challenged = 0;
                     client.user->is_challenging = 0;
                     challenger->is_challenged = 0;

                     write_client(find_client(clients, nb_clients, challenger)->sock, "game_refused");
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

static Client *find_client(Client *clients, int nb_clients, User *user)
{
   for (int i = 0; i < nb_clients; i++)
   {
      if (clients[i].user == user)
      {
         return &clients[i];
      }
   }
   return NULL;
}

static User *find_user(User *users, int nb_users, char *username)
{
   for (int id = 0; id < nb_users; ++id)
   {
      if (!strcmp(username, users[id].name))
      {
         return &users[id];
      }
   }
   return NULL;
}

static User *connect_user(User *users, int *nb_users, char *username)
{
   if (strlen(username) >= USERNAME_LENGTH || strchr(username, ',') != NULL || strchr(username, ':') != NULL)
   {
      return NULL;
   }
   User *user = find_user(users, *nb_users, username);
   if (user == NULL && *nb_users < MAX_USERS)
   {
      user = &users[*nb_users];
      (*nb_users)++;
      strcpy(user->name, username);
      user->is_connected = 1;
      user->is_playing = 0;
   }
   else
   {
      if (user->is_connected == 1)
      {
         return NULL;
      }
      user->is_connected = 1;
   }
   return user;
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
   clients[to_remove].user->is_connected = 0;
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
            strncat(message, source.user->name, BUF_SIZE - 1);
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
         strncat(message, clients[i].user->name, sizeof message - strlen(message) - 1);
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
