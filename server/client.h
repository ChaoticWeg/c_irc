#pragma once


#define CLIENT_INACTIVE  0
#define CLIENT_ACTIVE    1


struct client_t
{

    int active;
    
    pthread_t thread_id;
    int sockfd;

};
