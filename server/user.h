#ifndef USER_H
#define USER_H

#define USERNAME_LENGTH 16

typedef char Username[USERNAME_LENGTH];

typedef enum UserState
{
    FREE = 0,
    PLAYING = 1,
    WAITING_MOVE = 2,
    CHALLENGED = 3,
    CHALLENGING = 4
}UserState;

struct User
{
    Username name;
    int is_connected;

    struct User* opponent;
    UserState state;
} ;
typedef struct User User;

#endif