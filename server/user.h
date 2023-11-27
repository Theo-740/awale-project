#ifndef USER_H
#define USER_H

#define USERNAME_LENGTH 16

typedef char Username[USERNAME_LENGTH];

typedef struct User
{
    Username name;
    int is_connected;
    int is_playing;
    int is_challenged;
    int is_challenging;
    Username challenged;
} User;


#endif