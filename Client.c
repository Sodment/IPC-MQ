//
//  inf145330_k.c
//  IPC-PSIW
//
//  Created by Paweł Koch on 14/01/2021.
//

#include "inf145330_n.h"
key_t my_key;
int msg_id;
int my_control_sum;

// Function to calculate the user ID by adding ASCII value of characherts multiplied by their positions, creating semi uniqe ID
// returns value of user ID
int calculate_user_control_sum(User us)
{
    int sum = 0;
    for( int i = 0; i < sizeof(us.username); i++){
        int char_value_user = (int)us.username[i];
        int char_value_password = (int)us.password[i];
        // Control sum defines user and his password making ksemi uniqe number representation
        // of user for easier storing/comparing bassicly everything
        sum += ((char_value_user * i) + (char_value_password * 15 - i));
    }
    return sum;
}

// Reading username and password from stdin
// returns contorl sum of provided username and password
int read_credintenials_from_stdin()
{
    // fflush is unsecure and not recomended in doing in stdin but here we need to make sure we DONT have anything in in buffer
    fflush(stdin);
    char username[16] = {0};
    char password[16] = {0};
    printf("Login: ");
    if(fgets(username, 16, stdin) ==  NULL){
        printf("An error occured when trying to read username, please try again\n");
        return 1;
    }
    // Moving pointer of file to last char so we dont get 2 reads from 1 line of input
    // also it is much more secure than fflush
    fseek(stdin, 0, SEEK_END);
    printf("Password: ");
    if(fgets(password, 16, stdin) == NULL){
        printf("An error occured when trying to read password, please try again\n");
        return 1;
    }
    User user = {0, {0}, {0}};
    strcpy(user.username, username);
    strcpy(user.password, password);
    user.control_sum = calculate_user_control_sum(user);
    // we return only the control sum as its enough to identify user
    return user.control_sum;
}

// Function for login communication with the server side, simple sending request via message queue and waiting 2 seconds for it to reply then moving forward
// returns 0 if login is succseful, 1 otherwise
int user_login()
{
    int control_sum = read_credintenials_from_stdin();
    login_message.msg_type = USER_LOGIN;
    login_message.user_control_sum = control_sum;
    msgsnd(msg_id, &login_message, sizeof(login_message), 0);
    printf("Request for login has been sent to the server\n");
    // We wait for 2 seconds if server is busy with a lot of requests from other users
    sleep(2);
    // Instead of waiting i use IPC_NOWAIT to skip if message get stuck and i can still read 2 diffrent types of messages if needed (but server should only sent 1 kind of message, the other should not exist)
    msgrcv(msg_id, &login_message, sizeof(login_message), LOGIN_SUCCESFUL, IPC_NOWAIT);
    msgrcv(msg_id, &login_message, sizeof(login_message), BAD_LOGIN, IPC_NOWAIT);
    if(login_message.msg_type == LOGIN_SUCCESFUL)
    {
        printf("\n\nLOGIN SUCCESFUL\n\n\n");
        my_control_sum = control_sum;
        return 0;
        
    }
    else if(login_message.msg_type == BAD_LOGIN)
    {
        printf("\n\nINCORRECT USERNAME OR PASSWORD\n\n\n");
        return 1;
        
    }
    return 1;
}

// Function for registretaion, almost idnetical to login but it is aksing for registretaion instead of login which is diffrent on server side of things
// returns 0 if registreation is succseful, 1 otherwise
int register_new_user()
{
    int control_sum = read_credintenials_from_stdin();
    login_message.msg_type = USER_REGISTER;
    login_message.user_control_sum = control_sum;
    msgsnd(msg_id, &login_message, sizeof(login_message), 0);
    printf("Request for registretaion has been sent to the server\n");
    //Again wating a bit if server is very busy
    sleep(2);
    msgrcv(msg_id, &login_message, sizeof(login_message), REGISTRATION_SUCCESFUL, IPC_NOWAIT);
    msgrcv(msg_id, &login_message, sizeof(login_message), USER_EXISTS, IPC_NOWAIT);
    if (login_message.msg_type == REGISTRATION_SUCCESFUL) {
        printf("\n\nUser registered succesfully, you can now login into new account\n\n\n");
        return 0;
    }
    else if( login_message.msg_type == USER_EXISTS)
    {
        printf("\n\nUser already exists, registration failed\n\n\n");
        return 1;
    }
    return 1;
    
}

// Sending a request for server to see if our message topic exists/doesnt exist
// returns 0 when topic exists in server list of topics, 1 otherwise
int check_message_type(int type)
{
    if(type < FIRST_FREE_TOPIC)
    {
        printf("This topic number is impossible to send to");
        return 1;
    }
    mess_topic.msg_type = TOPIC_CHECK;
    mess_topic.topic = type;
    mess_topic.control_sum = my_control_sum;
    mess_topic.type_of_sub = -1;
    msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
    // Again wating a bit for response
    sleep(2);
    msgrcv(msg_id, &mess_topic, sizeof(mess_topic), TOPIC_FREE, IPC_NOWAIT);
    msgrcv(msg_id, &mess_topic, sizeof(mess_topic), TOPIC_EXISTS, IPC_NOWAIT);
    if(mess_topic.msg_type == TOPIC_FREE)
    {
        printf("\nTopic: %d does not exist\n", mess_topic.topic);
        return 1;
        
    }
    else if(mess_topic.msg_type == TOPIC_EXISTS)
    {
        printf("\nTopic: %d exists\n", mess_topic.topic);
        return 0;
    }
    return 1;
}

// Sending message by request
// returns 0 on succesfully sending message to server and recieving server approval of message, 1 otherwise
int send_messsage(int type)
{
    // First we asser that the topic of message exists on the server side
    if (check_message_type(type) == 0)
    {
        // Then we read and copy the memory block to message text
        char msg[MSG_SIZE];
        fflush(stdin);
        fseek(stdin, 0, SEEK_END);
        printf("Write your message: \n");
        if(fgets(msg, MSG_SIZE, stdin) == NULL)
        {
            printf("\nSomething went wrong when trying to read your message, please try again\n");
            return 1;
        }
        message.msg_type = type;
        strcpy(message.msg_text, msg);
        // Setting parameters for the struct that will be sent first to give server info about our message in &message
        mess_topic.msg_type = NEW_MESSAGE;
        mess_topic.topic = type;
        mess_topic.control_sum = my_control_sum;
        mess_topic.type_of_sub = -1;
        msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
        msgsnd(msg_id, &message, sizeof(message), 0);
        printf("\nRequested server to send message with topic %d\n", mess_topic.topic);
        sleep(2);
        msgrcv(msg_id, &mess_topic, sizeof(mess_topic), MESSAGE_SEND, IPC_NOWAIT);
        msgrcv(msg_id, &mess_topic, sizeof(mess_topic), MESSAGE_FAIL, IPC_NOWAIT);
        if (mess_topic.msg_type == MESSAGE_SEND)
        {
            printf("\n\nMessage sent succesfully!\n\n\n");
            return 0;
        }
        else if(mess_topic.msg_type == MESSAGE_FAIL)
        {
            printf("\n\nMessage passing failed, check server logs for more info\n\n\n");
            return 1;
        }
    }
    return 1;
}

// Sending request to server for creation of a new topic, can result in error that is handeld
// returns 0 if succesfully topic is added, 1 otherwise
int create_topic(int type, int sub_type)
{
    mess_topic.msg_type = CREATE_TOPIC;
    mess_topic.topic = type;
    mess_topic.control_sum = my_control_sum;
    mess_topic.type_of_sub = sub_type;
    msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
    printf("\n\nRequest to create topic %d has been sent to the server\n\n", mess_topic.topic);
    sleep(2);
    msgrcv(msg_id, &mess_topic, sizeof(mess_topic), TOPIC_CRETED, IPC_NOWAIT);
    msgrcv(msg_id, &mess_topic, sizeof(mess_topic), TOPIC_ADD_FAIL, IPC_NOWAIT);
    if (mess_topic.msg_type == TOPIC_CRETED) {
        printf("\n\nTopic %d created succesfully\n\n", mess_topic.topic);
        printf("\n\nYou have been added as a subscriber to topic: %d\n\n\n", mess_topic.topic);
        return 0;
    }
    else if( mess_topic.msg_type == TOPIC_ADD_FAIL)
    {
        printf("\nTrying to add topic %d resulted in a fail check logs to see what happend\n\n\n", mess_topic.topic);
        return 1;
    }
    return 1;
}

// Sending request to add and user to a subscriber list in topics list
// returns 0 on succsefully adding subscriber, 1 otherwise
int subscribe_to_new_message_type(int type, int sub_type)
{
    if(check_message_type(type) == 0)
    {
        mess_topic.msg_type = NEW_SUBSCRIBER;
        mess_topic.control_sum = my_control_sum;
        mess_topic.type_of_sub = sub_type;
        mess_topic.topic = type;
        msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
        printf("\n\nA request to subscribe to topic: %d has been sent to the server\n\n\n", mess_topic.topic);
        msgrcv(msg_id, &mess_topic, sizeof(mess_topic), ADDED_SUBSCIRBER, IPC_NOWAIT);
        msgrcv(msg_id, &mess_topic, sizeof(mess_topic), SUBSCIRBER_ADD_FAIL, IPC_NOWAIT);
        if (mess_topic.msg_type == ADDED_SUBSCIRBER) {
            printf("\n\nTopic %d created succesfully\n\n", mess_topic.topic);
            printf("\n\nYou have been added as a subscriber to topic: %d\n\n\n", mess_topic.topic);
            return 0;
        }
        else if( mess_topic.msg_type == SUBSCIRBER_ADD_FAIL)
        {
            printf("\nTrying to add subscriber %d to topic: %d resulted in a fail\n\n\n", mess_topic.control_sum, mess_topic.topic);
            return 1;
        }
    }
    return 1;
}

// Reading messages if available
// returns 0 on succesful reading, 1 otherwise
int read_message()
{
    char msg[MSG_SIZE];
    if(msgrcv(msg_id, &message, sizeof(message), my_control_sum, IPC_NOWAIT) != -1)
    {
        strcpy(msg, message.msg_text);
        printf("\nYou got new message:\n%s\n", msg);
        return 0;
    }
    else
    {
        printf("\n\nYou got no new messages\n\n\n");
        return 1;
    }
    return 1;
}

// Reading type from input
// returns type if succesful, -1 otherwise
int read_type_from_input()
{
    fseek(stdin, 0, SEEK_END);
    printf("\n\nMessage type:");
    int type;
    // We lack any error handling if scanf but patching every error of scnaf is imposibble in C99 so i guess we are fine for now
    scanf("%d", &type);
    fseek(stdin, 0, SEEK_END);
    if (type >= FIRST_FREE_TOPIC && type <= LAST_FREE_TOPIC) {
        return type;
    }
    return -1;
}

// Reading the subscytpion type that user want for his topic
// returns type of subcription, -2 otherwise
int read_subscryption()
{
    //Similar to login and function read_type_from_input() handling of typing
    fseek(stdin, 0, SEEK_END);
    printf("Subscription type: \n1.Permament\n2.For X messages\n");
    int choice = 0;
    char ch;
    ch = getchar();
    choice = ch - '0';
    fseek(stdin, 0, SEEK_END);
    switch (choice) {
        case 1:
        {
            return -1;
            break;
        }
        case 2:
        {
            printf("Amount of messages to recieve: ");
            unsigned x;
            scanf("%d", &x);
            fseek(stdin, 0, SEEK_END);
            return x;
        }
        default:
        {
            printf("\nOopps, something went wrong please try again\n");
            break;
        }
    }
    return -2;
}

// User interfance for client side, it handles everything we see on screen, doesnt handle a lot of errors
void client_interface(int synchronous_read)
{
    int not_logged = 1;
    int login_attempts = 0;
    while(not_logged)
    {
        int choice = 0;
        printf("---Please select an option---\n");
        printf("1. Log in into existing account\n");
        printf("2. Register a new account\n");
        printf("3. Exit program\n");
        char ch;
        printf("Your choice: ");
        ch = getchar();
        choice = ch - '0';
        fseek(stdin, 0, SEEK_END);
        switch (choice)
        {
            case 1:
            {
                not_logged = user_login();
                if(++login_attempts >= 5)
                {
                    printf("It seems you tried to login a couple of times without succes\n please wait 10 seconds before next attempt\n");
                    sleep(10);
                    login_attempts = 0;
                }
                break;
            }
            case 2:
            {
                if (register_new_user())
                {
                    printf("Something went wrong with registartion please try again\n");
                }
                break;
            }
            case 3:
            {
                printf("\n\n\n\n\nBye bye!\n\n\n\n\n");
                exit(0);
            }
            default:
            {
                printf("\nOopps, something went wrong please try again\n");
                break;
            }
        }
    }
    int run = 1;
    while (run) {
        int choice = 0;
        printf("---Please select an option---\n");
        printf("1. Send a new message\n");
        printf("2. Create new message type and subsrcibe to it\n");
        printf("3. Sybscribe to a new message type\n");
        if (synchronous_read)
        {
            printf("4. Read one message from subscribed topics\n");
        }
        printf("5. Exit program\n");
        char ch;
        printf("Your choice: ");
        ch = getchar();
        choice = ch - '0';
        fseek(stdin, 0, SEEK_END);
        switch (choice)
        {
            case 1:
            {
                int t = read_type_from_input();
                send_messsage(t);

                break;
            }
            case 2:
            {
                int t = read_type_from_input();
                int s = read_subscryption();
                create_topic(t, s);

                break;
            }
            case 3:
            {
                int t = read_type_from_input();
                int s = read_subscryption();
                subscribe_to_new_message_type(t, s);
                break;
            }
            case 4:
            {
                
                read_message();
                break;
            }
            case 5:
            {
                printf("\n\n\n\n\nBye bye!\n\n\n\n\n");
                exit(0);
            }
            default:
            {
                printf("\nOopps, something went wrong please try again\n");
                break;
            }
        }
    }
}



int main(int argc, char * argv[]){
    my_key = ftok("progfile", 66);
    msg_id = msgget(my_key, 0666 | IPC_CREAT);
    client_interface(1);
    return 0;
}
