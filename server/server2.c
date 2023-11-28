#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "server2.h"

static void init(void)
{
   srand(time(NULL));
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

static Client clients[MAX_CLIENTS];
static int nb_clients;

static User users[MAX_USERS];
static int nb_users;

static AwaleRunningGame awale_running_games[MAX_PLAYING_GAME];
static int nb_awale_running;

static AwaleStoredGame awale_stored_games[MAX_STORED_GAME];
static int nb_awale_stored;

static void app(void)
{
   SOCKET sock = init_connection();
   int max_fd = sock;
   char buffer[BUF_SIZE];

   nb_clients = 0;
   nb_users = 0;
   nb_awale_running = 0;
   nb_awale_stored = 0;
   /* an array for all clients */

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
         connect_client(sock, &max_fd, &rdfs);
      }
      else
      {
         int i = 0;
         for (i = 0; i < nb_clients; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client *client = &clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  disconnect_client(client);
               }
               else
               {
                  /*handle message*/
                  char *token = strtok(buffer, ":");
                  /* relay chat message*/
                  if (!strcmp(token, "chat"))
                  {
                     send_chat_message_to_all_clients(client, buffer + 5, 0);
                  }
                  /* show player list */
                  else if (!strcmp(token, "user_list"))
                  {
                     send_user_list_to_client(client);
                  }
                  /* challenge a user if didn't already asked someone */
                  else if (!strcmp(token, "challenge") && client->user->state == 0)
                  {
                     challenge_user(client);
                  }
                  /* accept a challenge */
                  else if (!strcmp(token, "game_accepted"))
                  {
                     accept_challenge(client);
                  }
                  /*refuse challenge*/
                  else if (!strcmp(token, "game_refused"))
                  {
                     refuse_challenge(client);
                  }
                  /*cancel challenge*/
                  else if (!strcmp(token, "cancel_challenge"))
                  {
                     cancel_challenge(client);
                  }
                  else if (!strcmp(token, "game_state"))
                  {
                     AwaleRunningGame *game = find_awale_running(client->user);

                     send_game(client, game);
                  }
                  else if (!strcmp(token, "move"))
                  {
                     token = strtok(NULL, "\n");
                     int move;
                     sscanf(token, "%d", &move);
                     AwaleRunningGame *game = find_awale_running(client->user);

                     if (
                         client->user->state == PLAYING)
                     {
                        int move_awale = awale_play_move(game, move);

                        if (move_awale < 0)
                        {
                           send_game(client, game);
                        }
                        /* game over */
                        else if (move_awale == 1 || move_awale == 2)
                        {
                           User *opponent_user = (game->player0 != client->user) ? game->player0 : game->player1;
                           Client *opponent_client = find_client(opponent_user);

                           send_winner_game(client, game);
                           send_winner_game(opponent_client, game);

                           // create an awale stored
                           //  put awale in the awale stored
                           // AwaleStoredGame* stored_game = store_awale_game(game, awale_stored, &nb_awale_stored);

                           client->user->state = FREE;
                           opponent_user->state = FREE;
                        }
                        else
                        {
                           User *opponent_user = (game->player0 != client->user) ? game->player0 : game->player1;
                           Client *opponent_client = find_client(opponent_user);
                           client->user->state = WAITING_MOVE;
                           opponent_user->state = PLAYING;
                           sprintf(buffer, "move:%d", move);
                           write_client(opponent_client->sock, buffer);
                        }
                     }
                  }
                  else if (!strcmp(token, "withdraw"))
                  {
                     AwaleRunningGame *game = find_awale_running(client->user);
                     game->winner = (client->user == game->player0) ? 1 : 0;
                     User *opponent_user = (game->player0 != client->user) ? game->player0 : game->player1;
                     Client *opponent_client = find_client(opponent_user);

                     write_client(opponent_client->sock, "withdrew");

                     // create an awale stored
                     //  put awale in the awale stored
                     // AwaleStoredGame* stored_game = store_awale_game(game, awale_stored, &nb_awale_stored);

                     client->user->state = FREE;
                     opponent_user->state = FREE;
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
#ifdef DEBUG
   printf("sock n°%d says : \"%s\"\n", sock, buffer);
#endif
   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }

#ifdef DEBUG
   printf("I say : \"%s\"\n", buffer);
#endif
}

static void connect_client(SOCKET sock, int *max_fd, fd_set *rdfs)
{
   char buffer[BUF_SIZE];
   SOCKADDR_IN csin = {0};
   socklen_t sinsize = sizeof csin;
   int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
   if (csock == SOCKET_ERROR)
   {
      perror("accept()");
      return;
   }

   /* after connecting the client sends its name */
   if (read_client(csock, buffer) == -1)
   {
      return;
   }

   User *user = connect_user(buffer);
   if (user == NULL)
   {
      write_client(csock, "nope");
      closesocket(csock);
   }
   else
   {
      /* what is the new maximum fd ? */
      *max_fd = csock > *max_fd ? csock : *max_fd;

      FD_SET(csock, rdfs);

      // strncpy(c.name, buffer, BUF_SIZE - 1);
      Client *c = &clients[nb_clients];
      nb_clients++;
      c->sock = csock;
      c->user = user;

      if (c->user->state == PLAYING || c->user->state == WAITING_MOVE)
      {
         write_client(csock, "game");
      }
      else if (c->user->state == CHALLENGING)
      {
         strcpy(buffer, "challenging:");
         strcat(buffer, c->user->opponent->name);
         write_client(csock, buffer);
      }
      else if (c->user->state == CHALLENGED)
      {
         strcpy(buffer, "challenged:");
         strcat(buffer, c->user->opponent->name);

         write_client(csock, buffer);
      }
      else
      {
         write_client(csock, "menu");
      }

      /*strncpy(buffer, c->user->name, BUF_SIZE - 1);
      strncat(buffer, " connected !", BUF_SIZE - strlen(buffer) - 1);
      send_message_to_all_clients(clients, *c, nb_clients, users, buffer, 1);*/
   }
}

static void disconnect_client(Client *client)
{
   // char buffer[BUF_SIZE];
   User *disconnected_user = client->user;
   disconnected_user->is_connected = 0;
   closesocket(client->sock);
   remove_client(client);

   /*strncpy(buffer, disconnected_user->name, BUF_SIZE - 1);
   strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
   send_message_to_all_clients(clients, client, nb_clients, users, buffer, 1);*/
}

static void challenge_user(Client *challenger)
{
   if (challenger->user->state != FREE)
   {
      return;
   }
   char buffer[BUF_SIZE];
   char *token = strtok(NULL, ";");
   User *challenged_user = find_user(token);
   if (challenged_user != NULL && challenged_user->is_connected && challenged_user->state == FREE)
   {
      Client *challenged_client = find_client(challenged_user);
      strncpy(buffer, "challenged:", BUF_SIZE - 1);
      strncat(buffer, challenger->user->name, BUF_SIZE - strlen(buffer) - 1);
      write_client(challenged_client->sock, buffer);
      challenger->user->state = CHALLENGING;
      challenged_user->state = CHALLENGED;
      challenger->user->opponent = challenged_user;
      challenged_user->opponent = challenger->user;
   }
   else
   {
      write_client(challenger->sock, "game_refused");
   }
}

static void accept_challenge(Client *client)
{
   if (client->user->state != CHALLENGED)
   {
      return;
   }
   User *opponent_user = client->user->opponent;
   Client *opponent_client = find_client(opponent_user);
   if (opponent_client != NULL)
   {
      write_client(opponent_client->sock, "game_accepted");
   }

   User *first_player;
   User *second_player;
   if (rand() % 2)
   {
      first_player = client->user;
      second_player = opponent_user;
   }
   else
   {
      first_player = opponent_user;
      second_player = client->user;
   }

   first_player->state = PLAYING;
   second_player->state = WAITING_MOVE;

   // create new awale running
   AwaleRunningGame *game = &awale_running_games[nb_awale_running];
   awale_init_game(game);
   game->player0 = first_player;
   game->player1 = second_player;
   nb_awale_running++;
}

static void refuse_challenge(Client *client)
{
   if (client->user->state != CHALLENGED)
   {
      return;
   }
   User *challenger = client->user->opponent;
   client->user->state = FREE;
   challenger->state = FREE;

   Client *challenger_client = find_client(challenger);
   if (challenger_client != NULL)
   {
      write_client(challenger_client->sock, "game_refused");
   }
}

static void cancel_challenge(Client *client)
{
   if (client->user->state != CHALLENGING)
   {
      return;
   }
   User *challenged = client->user->opponent;
   client->user->state = FREE;
   challenged->state = FREE;

   Client *challenged_client = find_client(challenged);
   if (challenged_client != NULL)
   {
      write_client(challenged_client->sock, "challenge_canceled");
   }
}

static void send_game(Client *client, AwaleRunningGame *game)
{
   char message[BUF_SIZE];
   sprintf(message, "game_state:{you:%d,turn:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}\n",
           (client->user == game->player0) ? 0 : 1,
           game->nbTurns % 2,
           game->board[0],
           game->board[1],
           game->board[2],
           game->board[3],
           game->board[4],
           game->board[5],
           game->board[6],
           game->board[7],
           game->board[8],
           game->board[9],
           game->board[10],
           game->board[11],
           game->scores[0],
           game->scores[1]);

   write_client(client->sock, message);
}

static void send_winner_game(Client *client, AwaleRunningGame *game)
{
   char message[BUF_SIZE];
   sprintf(message, "game_end:{you:%d,winner:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d}\n",
           (client->user == game->player0) ? 0 : 1,
           game->winner,
           game->board[0],
           game->board[1],
           game->board[2],
           game->board[3],
           game->board[4],
           game->board[5],
           game->board[6],
           game->board[7],
           game->board[8],
           game->board[9],
           game->board[10],
           game->board[11],
           game->scores[0],
           game->scores[1]);

   write_client(client->sock, message);
}

static AwaleRunningGame *find_awale_running(User *user)
{
   for (int i = 0; i < nb_awale_running; ++i)
   {
      if (awale_running_games[i].player0 == user || awale_running_games[i].player1 == user)
      {
         return &awale_running_games[i];
      }
   }
   return NULL;
}

static AwaleStoredGame *store_awale_game(AwaleRunningGame *game)
{
   return NULL;
}

static void print_all_users()
{
   for (int i = 0; i < nb_users; ++i)
   {
      printf("%d %s\n", i, users[i].name);
   }
}

static Client *find_client(User *user)
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

static User *find_user(char *username)
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

static User *connect_user(char *username)
{
   if (strlen(username) >= USERNAME_LENGTH || strchr(username, ',') != NULL || strchr(username, ':') != NULL || strchr(username, ';') != NULL)
   {
      return NULL;
   }
   User *user = find_user(username);
   if (user == NULL && nb_users < MAX_USERS)
   {
      user = &users[nb_users];
      (nb_users)++;
      strcpy(user->name, username);
      user->is_connected = 1;
      user->state = 0;
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

static void clear_clients()
{
   int i = 0;
   for (i = 0; i < nb_clients; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *client)
{
   /* we remove the client in the array */
   memmove(client, client + 1, (nb_clients - 1) * sizeof(Client) - (client - clients));
   /* number client - 1 */
   nb_clients--;
}

static void send_chat_message_to_all_clients(Client *source, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   strncpy(message, "chat:", BUF_SIZE - 1);
   for (i = 0; i < nb_clients; i++)
   {
      /* we don't send message to the source */
      if (source != &clients[i])
      {
         if (from_server == 0)
         {
            strncat(message, source->user->name, BUF_SIZE - 1);
            strncat(message, ": ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static void send_user_list_to_client(Client *target)
{
   int i = 0;
   char message[BUF_SIZE];
   strncpy(message, "user_list:", BUF_SIZE - 1);
   for (i = 0; i < nb_clients; i++)
   {
      /* we don't send his name to the target */
      if (target != &clients[i])
      {
         strncat(message, clients[i].user->name, sizeof message - strlen(message) - 1);
         strncat(message, ",", sizeof message - strlen(message) - 1);
      }
   }
   write_client(target->sock, message);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
