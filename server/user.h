#ifndef USER_H
#define USER_H

#define USERNAME_LENGTH 16

typedef char username_t[USERNAME_LENGTH];

typedef struct user_t
{
    username_t name;
    int is_connected;
    int is_playing;
} user_t;


#endif