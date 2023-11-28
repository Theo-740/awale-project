#ifndef USER_H
#define USER_H

#define USERNAME_LENGTH 16

typedef char Username[USERNAME_LENGTH];

typedef enum UserState
{
    FREE = 0,
    PLAYING,
    WAITING_MOVE,
    CHALLENGED,
    CHALLENGING
}UserState;

struct User
{
    Username name;
    int is_connected;

    struct User* challenger;
    UserState state;
} ;
typedef struct User User;

#endif