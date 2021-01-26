//
//  inf145330_n.h
//  IPC-PSIW
//
//  Created by Paweł Koch on 14/01/2021.
//

#ifndef inf145330_n_h
#define inf145330_n_h
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Buffor for txt messages
#define MSG_SIZE 512

//Signals that we predefined, not every is used i suppose but if needed later they will exist here
#define USER_REGISTER 3
#define USER_LOGIN 4
#define BAD_LOGIN 5
#define LOGIN_SUCCESFUL 6
#define REGISTRATION_SUCCESFUL 7
#define USER_EXISTS 8
#define TOPIC_CHECK 9
#define TOPIC_EXISTS 10
#define TOPIC_FREE 11
#define NEW_MESSAGE 12
#define NEW_SUBSCRIBER 13
#define CREATE_TOPIC 14
#define ADDED_SUBSCIRBER 15
#define SUBSCIRBER_ADD_FAIL 16
#define MESSAGE_FAIL 17
#define MESSAGE_SEND 18
#define TOPIC_CRETED 19
#define TOPIC_ADD_FAIL 20
#define FIRST_FREE_TOPIC 21
#define LAST_FREE_TOPIC 39

// Struct for txt message sending
struct mess_buff {
    long msg_type;
    char msg_text[MSG_SIZE];
} message;


// Struct for login queue and registration
struct login_buff {
    long msg_type;
    int user_control_sum;
} login_message;

// Struct for topic realted sending/reciving
struct mess_topic_buff{
    long msg_type;
    int control_sum;
    int type_of_sub;
    int topic;
} mess_topic;

// quick struct for having user data in one place
typedef struct Users{
    int control_sum;
    char username[16];
    char password[16];
} User;

// struct for keeping users subscribed to topic in one place with their subsribition
typedef struct Subscriber{
    int control_sum;
    int type_of_sub;
} Sub;

// struct for keeping topics in rewadeable form
typedef struct Topic{
    Sub topic_subscribers[2048];
    int exists;
} Top;


#endif
