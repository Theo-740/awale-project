#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include "client2.h"
#include "awale.h"
#include "controller.h"

void init(void)
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

void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

void app(const char *address, const char *name)
{
   printf("client starting...\n");
#ifdef DEBUG
   printf("debug mode activated\n");
#endif
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];
   Controller ctrl;

   fd_set rdfs;

   controller_init(&ctrl, sock, name);

   while (ctrl.state != TERMINATED)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         fgets(buffer, BUF_SIZE - 1, stdin);
         {
            char *p = NULL;
            p = strstr(buffer, "\n");
            if (p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }
         controller_user_input(&ctrl, buffer);
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if (n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }
         char *messages[MAX_MESSAGES];
         messages[0] = strtok(buffer, ";");
         int nb_messages = 1;
         while (nb_messages < MAX_MESSAGES && (messages[nb_messages] = strtok(NULL, ";")) != NULL)
         {
            nb_messages++;
         }
         for (int i = 0; i < nb_messages; i++)
         {
            controller_server_input(&ctrl, messages[i]);
         }
      }
   }

   end_connection(sock);
}

int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};
   struct hostent *hostinfo;

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf(stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

void end_connection(int sock)
{
   closesocket(sock);
}

int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;
#ifdef DEBUG
   printf("server says : \"%s\"\n", buffer);
#endif
   return n;
}

void write_server(SOCKET sock, const char *buffer)
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

int main(int argc, char **argv)
{
   if (argc != 3)
   {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}
