/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           client.h
    Description:    Defines struct client_t, a structure to hold information about active clients
    
*/

#pragma once


// activity states (work like booleans)
#define CLIENT_INACTIVE  0
#define CLIENT_ACTIVE    1


/** Hold info about active clients */
struct client_t
{

    int active;           // is the client currently active?
    
    pthread_t thread_id;  // pthread pid for this client
    int sockfd;           // INET socket file descriptor for this client

};
