#ifndef USER_H
#define USER_H

#define USERNAME_LENGTH 16

typedef char Username[USERNAME_LENGTH];

typedef struct User
{
    Username name;
    int is_connected;
    int is_playing;
} User;


#endif