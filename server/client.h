/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           client.h
    Description:    Defines struct client_t, a structure to hold information about active clients
    
*/

#pragma once


#define CLIENT_INACTIVE  0
#define CLIENT_ACTIVE    1


struct client_t
{

    int active;
    
    pthread_t thread_id;
    int sockfd;

};
