//
//  inf145330_s.c
//  IPC-PSIW
//
//  Created by Paweł Koch on 14/01/2021.
//
#include "inf145330_n.h"

//Maximum number of users to handle
int users[2048] = {0};
//Maximum number of topics
Top topics[40];
key_t my_key;
int msg_id;

// Check if user exist for login/registration
// returns 1 if user exists 0 otherwise
int user_exists(int control_sum)
{
    for (int i = 0; i < 2048; i++)
    {
        if(users[i] == control_sum)
        {
            return 1;
        }
    }
    return 0;
}

// Function that answers to USER_LOGIN message from queue
// returns 0 if login was succesful, 1 otherwise
int login_verificataion()
{
    printf("NEW LOGIN ATTEMPT BY %D\n" , login_message.user_control_sum);
    int logged = 0;
    for (int i = 0; i < 2048; i++)
    {
        if (login_message.user_control_sum == users[i])
        {
            logged = 1;
            break;
        }
        else
        {
            continue;
        }
    }
    
    if(logged)
    {
        login_message.msg_type = LOGIN_SUCCESFUL;
        msgsnd(msg_id, &login_message, sizeof(login_message), 0);
        printf("Login attempt by %d was succesful\n", login_message.user_control_sum);
        return 0;
    }
    else
    {
        login_message.msg_type = BAD_LOGIN;
        msgsnd(msg_id, &login_message, sizeof(login_message), 0);
        printf("Login attempt by %d was  NOT succesful\n", login_message.user_control_sum);
        return 1;
    }
    return 1;
    
}

// Function that answers to USER_REGISTER message from queue
// returns 0 if registreation was succesful, 1 otherwise
int register_new_users()
{
    printf("NEW REGISTARTION ATTEMPT BY %D\n" , login_message.user_control_sum);
    if(user_exists(login_message.user_control_sum))
    {
        login_message.msg_type = USER_EXISTS;
        msgsnd(msg_id, &login_message, sizeof(login_message), 0);
        return 1;
    }
    for (int i = 0; i < 2048; i++)
    {
        if(users[i] == 0)
        {
            users[i] = login_message.user_control_sum;
            printf("New user: %d has been registered\n", login_message.user_control_sum);
            login_message.msg_type = REGISTRATION_SUCCESFUL;
            msgsnd(msg_id, &login_message, sizeof(login_message), 0);
            return 0;
        }
        else
        {
            continue;
        }
    }
    return 1;
}

// Function checks if topic already exists and rpvides a signal for client with answer
// returns 0 if topic doesnt exist, 1 if topic exists and -1 if an error occured
int check_topics_existence()
{
    printf("NEW TOPIC CHECK ATEMPT BY: %d WITH TOPIC: %d\n", mess_topic.control_sum, mess_topic.topic);
    int topic_index = mess_topic.topic - FIRST_FREE_TOPIC;
        if(topics[topic_index].exists == 0)
        {
            mess_topic.msg_type = TOPIC_FREE;
            msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
            printf("TOPIC: %D IS FREE\n", mess_topic.topic);
            return 0;
        }
        else if(topics[topic_index].exists == 1)
        {
            mess_topic.msg_type = TOPIC_EXISTS;
            msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
            printf("TOPIC: %D EXISTS\n", mess_topic.topic);
            return 1;
        }
    return -1;
}

// Function that answers for message request from clients, it responds with sending message to all subscribers
// returns 0 if sending was succesful, 1 otherwise
int recieve_and_resend()
{
    printf("NEW MESSAGE REQUEST BY %D, TOPIC %lD\n", mess_topic.control_sum, message.msg_type);
    if(check_topics_existence() == 1)
    {
        char msg[4096];
        msgrcv(msg_id, &message, sizeof(message), mess_topic.topic, IPC_NOWAIT);
        printf("NEW MESSAGE RECIEVED FROM %D\n", mess_topic.control_sum);
        int selected_topic = mess_topic.topic - FIRST_FREE_TOPIC;
        strcpy(msg, message.msg_text);
        for (int i = 0; i < 2048; i++) {
            if(topics[selected_topic].topic_subscribers[i].control_sum > 0)
            {
                if (topics[selected_topic].topic_subscribers[i].type_of_sub > 0
                    ||
                    topics[selected_topic].topic_subscribers[i].type_of_sub == -1)
                {
                    --topics[selected_topic].topic_subscribers[i].type_of_sub;
                    message.msg_type = topics[selected_topic].topic_subscribers[i].control_sum;
                    msgsnd(msg_id, &message, sizeof(message), 0);
                    mess_topic.msg_type = MESSAGE_SEND;
                    msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
                    printf("MESSAGE FROM %d HAS BEEN SENT TO %ld\n SUBSCRIBERS", mess_topic.control_sum, message.msg_type);
                }
            }
            else
            {
                break;
            }
        }
        return 0;
    }
    mess_topic.msg_type = MESSAGE_FAIL;
    msgsnd(msg_id, &message, sizeof(message), 0);
    return 1;
}


//Function adds user Id to subscirbers with his preffered subscipriton type
//returns 0 if adding was succesful, 1 otherwise
int add_to_subscribers()
{
    if(check_topics_existence() == 1)
    {
        int selected_topic = mess_topic.topic - FIRST_FREE_TOPIC;
        for (int i = 0; i < 2048; i++) {
            if(topics[selected_topic].topic_subscribers[i].control_sum == 0)
            {
                topics[selected_topic].topic_subscribers[i].control_sum = mess_topic.control_sum;
                topics[selected_topic].topic_subscribers[i].type_of_sub = mess_topic.type_of_sub;
                mess_topic.msg_type = ADDED_SUBSCIRBER;
                msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
                printf("USER %D has been added to subscribers of topic: %d\n", mess_topic.control_sum, mess_topic.topic);
                return 0;
            }
        }
    }
    mess_topic.msg_type = SUBSCIRBER_ADD_FAIL;
    msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
    return 1;
    
}

//Function creates new topics if space is avaiable
//returns 0 if creation of topic was succesful, 1 otherwise
int create_topic()
{
    if(check_topics_existence() == 0)
    {
        int selected_topic = mess_topic.topic - FIRST_FREE_TOPIC;
        topics[selected_topic].exists = 1;
        printf("TOPIC %D, has been created\n", mess_topic.topic);
        mess_topic.msg_type = TOPIC_CRETED;
        msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
        add_to_subscribers();
        return 0;
    }
    mess_topic.msg_type = TOPIC_ADD_FAIL;
    msgsnd(msg_id, &mess_topic, sizeof(mess_topic), 0);
    return 1;
}


int main(int argc, char * argv[]){
    my_key = ftok("progfile", 66);
    msg_id = msgget(my_key, 0666 | IPC_CREAT);
    printf("Server is starting\n.\n.\n.");
    printf("\nServer is ready to work!\n");
    // Checking if a particular signal occured, it can be split to thread but Im honestly too tired to think about creating thread and live_lock nightmare that can happen here
    while(1)
    {
        if(msgrcv(msg_id, &login_message, sizeof(login_message), USER_REGISTER, IPC_NOWAIT) != -1)
        {
            register_new_users();
        }
        if(msgrcv(msg_id, &login_message, sizeof(login_message), USER_LOGIN, IPC_NOWAIT) != -1)
        {
            login_verificataion();
        }
        if(msgrcv(msg_id, &mess_topic, sizeof(mess_topic), CREATE_TOPIC, IPC_NOWAIT) != -1)
        {
            create_topic();
        }
        if(msgrcv(msg_id, &mess_topic, sizeof(mess_topic), NEW_MESSAGE, IPC_NOWAIT) != -1)
        {
            recieve_and_resend();
        }
        if(msgrcv(msg_id, &mess_topic, sizeof(mess_topic), NEW_SUBSCRIBER, IPC_NOWAIT) != -1)
        {
            add_to_subscribers();
        }
        if(msgrcv(msg_id, &mess_topic, sizeof(mess_topic), TOPIC_CHECK, IPC_NOWAIT) != -1)
        {
            check_topics_existence();
        }
        sleep(1);
    }
}
