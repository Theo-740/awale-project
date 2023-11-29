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

static RunningGame running_games[MAX_PLAYING_GAMES];
static int nb_running_games;

static StoredGame stored_games[MAX_STORED_GAMES];
static int nb_stored_games;

static void app(void)
{
   printf("server starting...\n");
#ifdef DEBUG
   printf("debug mode activated\n");
#endif
   SOCKET sock = init_connection();
#ifdef DEBUG
   printf("opened socket n°%d\n", sock);
#endif
   int max_fd = sock;
   char buffer[BUF_SIZE];

   nb_clients = 0;
   nb_users = 0;
   nb_running_games = 0;
   nb_stored_games = 0;
   /* an array for all clients */

   fd_set rdfs;

   printf("server ready!\n");

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
                  char *messages[MAX_MESSAGES];
                  messages[0] = strtok(buffer, ";");
                  int nb_messages = 1;
                  while (nb_messages < MAX_MESSAGES && (messages[nb_messages] = strtok(NULL, ";")) != NULL)
                  {
                     nb_messages++;
                  }
                  for (int j = 0; j < nb_messages; j++)
                  {
                     /*handle message*/
                     char *content;
                     const char *header = strtok_r(messages[j], ":", &content);
                     /* relay chat message*/
                     if (!strcmp(header, "chat"))
                     {
                        send_chat_message_to_all_clients(client, content, 0);
                     }
                     /* show player list */
                     else if (!strcmp(header, "user_list"))
                     {
                        send_user_list_to_client(client);
                     }
                     /* show running game list */
                     else if (!strcmp(header, "running_game_list"))
                     {
                        send_list_running_games(client);
                     }
                     /* challenge a user if didn't already asked someone */
                     else if (!strcmp(header, "challenge") && client->user->state == 0)
                     {
                        challenge_user(client, content);
                     }
                     /* accept a challenge */
                     else if (!strcmp(header, "accept_challenge"))
                     {
                        accept_challenge(client);
                     }
                     /*refuse challenge*/
                     else if (!strcmp(header, "refuse_challenge"))
                     {
                        refuse_challenge(client);
                     }
                     /*cancel challenge*/
                     else if (!strcmp(header, "cancel_challenge"))
                     {
                        cancel_challenge(client);
                     }
                     else if (!strcmp(header, "game_state"))
                     {
                        RunningGame *game = find_running_game_by_player(client->user);
                        send_game(client, game);
                     }
                     else if (!strcmp(header, "move"))
                     {
                        make_move(client, content);
                     }
                     else if (!strcmp(header, "withdraw"))
                     {
                        RunningGame *game = find_running_game_by_player(client->user);
                        game->awale.infos.winner = (client->user == game->player0) ? 1 : 0;
                        User *opponent_user = (client->user == game->player0) ? game->player1 : game->player0;
                        Client *opponent_client = find_client(opponent_user);
                        if (opponent_client != NULL)
                        {
                           write_client(opponent_client->sock, "withdrew;");
                        }
                        client->user->state = FREE;
                        opponent_user->state = FREE;
                        // create an awale stored
                        //  put awale in the awale stored
                        // store_game(game);
                     }
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
#ifdef DEBUG
   printf("closed socket n°%d\n", sock);
#endif
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
   printf("I say to sock n°%d : \"%s\"\n", sock, buffer);
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
#ifdef DEBUG
   printf("opened socket n°%d\n", csock);
#endif

   /* after connecting the client sends its name */
   if (read_client(csock, buffer) == -1)
   {
      return;
   }

   User *user = connect_user(buffer);
   if (user == NULL)
   {
      write_client(csock, "nope;");
      closesocket(csock);
#ifdef DEBUG
      printf("closed socket n°%d\n", csock);
#endif
   }
   else
   {
      /* what is the new maximum fd ? */
      *max_fd = csock > *max_fd ? csock : *max_fd;

      FD_SET(csock, rdfs);

      // strncpy(c.name, buffer, BUF_SIZE - 1);
      Client *client = &clients[nb_clients];
      nb_clients++;
      client->sock = csock;
      client->user = user;

      if (client->user->state == PLAYING || client->user->state == WAITING_MOVE)
      {
         write_client(client->sock, "game;");
         RunningGame *game = find_running_game_by_player(client->user);
         add_observer(game, client);
      }
      else if (client->user->state == CHALLENGING)
      {
         strcpy(buffer, "challenging:");
         strncat(buffer, client->user->opponent->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, ";", BUF_SIZE - strlen(buffer) - 1);
         write_client(client->sock, buffer);
      }
      else if (client->user->state == CHALLENGED)
      {
         strcpy(buffer, "challenged:");
         strncat(buffer, client->user->opponent->name, BUF_SIZE - strlen(buffer) - 1);
         strncat(buffer, ";", BUF_SIZE - strlen(buffer) - 1);
         write_client(client->sock, buffer);
      }
      else
      {
         write_client(client->sock, "menu;");
      }

      /*strncpy(buffer, client->user->name, BUF_SIZE - 1);
      strncat(buffer, " connected !", BUF_SIZE - strlen(buffer) - 1);
      send_message_to_all_clients(clients, client, nb_clients, users, buffer, 1);*/
   }
}

static void disconnect_client(Client *client)
{
   // char buffer[BUF_SIZE];
   User *disconnected_user = client->user;
   disconnected_user->is_connected = 0;
   if (client->is_observing)
   {
      remove_observer(find_running_game_by_observer(client), client);
   }
   closesocket(client->sock);
#ifdef DEBUG
   printf("closed socket n°%d\n", client->sock);
#endif
   remove_client(client);

   /*strncpy(buffer, disconnected_user->name, BUF_SIZE - 1);
   strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
   send_message_to_all_clients(clients, client, nb_clients, users, buffer, 1);*/
}

static void challenge_user(Client *challenger, const char *username)
{
   if (challenger->user->state != FREE)
   {
      return;
   }
   char buffer[BUF_SIZE];
   User *challenged_user = find_user(username);
   if (challenged_user != NULL && challenged_user->is_connected && challenged_user->state == FREE)
   {
      Client *challenged_client = find_client(challenged_user);
      strncpy(buffer, "challenged:", BUF_SIZE - 1);
      strncat(buffer, challenger->user->name, BUF_SIZE - strlen(buffer) - 1);
      strncat(buffer, ";", BUF_SIZE - strlen(buffer) - 1);
      write_client(challenged_client->sock, buffer);
      challenger->user->state = CHALLENGING;
      challenged_user->state = CHALLENGED;
      challenger->user->opponent = challenged_user;
      challenged_user->opponent = challenger->user;
   }
   else
   {
      write_client(challenger->sock, "challenge_refused;");
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
      write_client(opponent_client->sock, "challenge_accepted;");
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
   RunningGame *game = &running_games[nb_running_games];
   awale_init_game(&game->awale);
   game->id = nb_running_games + nb_stored_games;
#ifdef DEBUG
   printf("%d\n", game->id);
#endif
   game->player0 = first_player;
   game->player1 = second_player;
   game->observers[0] = client;
   game->observers[1] = opponent_client;
   game->nb_observers = 2;
   nb_running_games++;
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
      write_client(challenger_client->sock, "challenge_refused;");
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
      write_client(challenged_client->sock, "challenge_canceled;");
   }
}

static void send_game(Client *client, RunningGame *game)
{
   char message[BUF_SIZE];
   char hole[BUF_SIZE];
   snprintf(message,
            BUF_SIZE - 1,
            "game_state:%s,%s,%d,%d,%d",
            game->player0->name,
            game->player1->name,
            game->awale.infos.scores[0],
            game->awale.infos.scores[1],
            game->awale.infos.nbTurns);
   for (int i = 0; i < AWALE_BOARD_SIZE; i++)
   {
      snprintf(hole, BUF_SIZE - 1, ",%d", game->awale.board[i]);
      strncat(message, hole, BUF_SIZE - strlen(message) - 1);
   }
   strncat(message, ";", BUF_SIZE - strlen(message) - 1);

   write_client(client->sock, message);
}

static void send_winner_game(Client *client, RunningGame *game)
{
   char message[BUF_SIZE];
   sprintf(message, "game_end:{you:%d,winner:%d,board:{%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd},scores:{%d,%d};",
           (client->user == game->player0) ? 0 : 1,
           game->awale.infos.winner,
           game->awale.board[0],
           game->awale.board[1],
           game->awale.board[2],
           game->awale.board[3],
           game->awale.board[4],
           game->awale.board[5],
           game->awale.board[6],
           game->awale.board[7],
           game->awale.board[8],
           game->awale.board[9],
           game->awale.board[10],
           game->awale.board[11],
           game->awale.infos.scores[0],
           game->awale.infos.scores[1]);

   write_client(client->sock, message);
}

static void make_move(Client *client, const char *move_description)
{
   char buffer[BUF_SIZE];
   if (client->user->state != PLAYING)
   {
      return;
   }
   int move;
   if (sscanf(move_description, "%d", &move) != 1)
   {
      return;
   }
   RunningGame *game = find_running_game_by_player(client->user);

   int result = awale_play_move(&game->awale, move);

   if (result < 0)
   {
      send_game(client, game);
   }
   else
   {
      User *opponent_user = (game->player0 != client->user) ? game->player0 : game->player1;
      /* game over */
      if (result > 0)
      {
         for (int i = 0; i < game->nb_observers; i++)
         {
            send_winner_game(game->observers[i], game);
         }

         store_game(game);

         client->user->state = FREE;
         opponent_user->state = FREE;
      }
      else
      {
         client->user->state = WAITING_MOVE;
         opponent_user->state = PLAYING;
         sprintf(buffer, "move:%d;", move);
         for (int i = 0; i < game->nb_observers; i++)
         {
            if (game->observers[i] != client)
            {
               write_client(game->observers[i]->sock, buffer);
            }
         }
      }
   }
}

static RunningGame *find_running_game_by_player(User *user)
{
   for (int i = 0; i < nb_running_games; ++i)
   {
      if (running_games[i].player0 == user || running_games[i].player1 == user)
      {
         return &running_games[i];
      }
   }
   return NULL;
}

static RunningGame *find_running_game_by_observer(Client *client)
{
   for (int i = 0; i < nb_running_games; ++i)
   {
      RunningGame *game = &running_games[i];
      for (int j = 0; j < game->nb_observers; j++)
      {
         if (game->observers[j] == client)
         {
            return game;
         }
      }
   }
   return NULL;
}

static StoredGame *store_game(RunningGame *game)
{
   StoredGame *stored = &stored_games[nb_stored_games++];
   stored->id = game->id;
   stored->player0 = game->player0;
   stored->player1 = game->player1;
   memcpy(&stored->awale, &game->awale.infos, sizeof(AwaleGameInfos));

   /* we remove the game in the array */
   memmove(game, game + 1, (nb_running_games - 1) * sizeof(RunningGame) - (game - running_games));
   /* number game running - 1 */
   nb_running_games--;
   return stored;
}

static void print_all_users()
{
   for (int i = 0; i < nb_users; ++i)
   {
      printf("%d %s\n", i, users[i].name);
   }
}

static Client *find_client(const User *user)
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

static User *find_user(const char *username)
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

static User *connect_user(const char *username)
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

static void add_observer(RunningGame *game, Client *observer)
{
   if (game->nb_observers == MAX_OBSERVERS)
   {
      return;
   }
   game->observers[game->nb_observers++] = observer;
}

static void remove_observer(RunningGame *game, Client *observer)
{
   for (int i = 0; i < game->nb_observers; i++)
   {
      if (game->observers[i] == observer)
      {
         memcpy(&game->observers[i], &game->observers[i + 1], (game->nb_observers - i - 1) * sizeof(Client));
      }
   }
}

static void clear_clients()
{
   int i = 0;
   for (i = 0; i < nb_clients; i++)
   {
      closesocket(clients[i].sock);
#ifdef DEBUG
      printf("closed socket n°%d\n", clients[i].sock);
#endif
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
         strncat(message, ";", sizeof message - strlen(message) - 1);
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
   message[strlen(message) - 1] = ';';
   write_client(target->sock, message);
}

static void send_list_running_games(Client *target)
{
   char message[BUF_SIZE];
   strncpy(message, "running_game_list:", BUF_SIZE - 1);
   for (int i = 0; i < nb_running_games; ++i)
   {
      char string_game_id[BUF_SIZE];
      snprintf(string_game_id, BUF_SIZE, "%d,", running_games[i].id);
      strncat(message, string_game_id, BUF_SIZE - strlen(message) - 1);
      strncat(message, running_games[i].player0->name, BUF_SIZE - strlen(message) - 1);
      strncat(message, ",", BUF_SIZE - strlen(message) - 1);
      strncat(message, running_games[i].player1->name, BUF_SIZE - strlen(message) - 1);
      strncat(message, ",", BUF_SIZE - strlen(message) - 1);
   }
   message[strlen(message) - 1] = ';';
   write_client(target->sock, message);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
