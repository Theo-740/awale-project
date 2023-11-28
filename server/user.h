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

typedef struct User
{
    Username name;
    int is_connected;

    Username challenged;
    UserState state;
} User;

#endif