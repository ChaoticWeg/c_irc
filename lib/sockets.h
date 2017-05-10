/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           sockets.h
    Description:    Provides helper methods regarding sockets
    
*/

#pragma once

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>   // constants
#include <netinet/tcp.h>  // TCP_NODELAY

#include <errno.h>   // strerror(), errno
#include <string.h>  // bzero()


// connection info
#define HOSTNAME         "cse01.cse.unt.edu"
#define PORT_NUMBER      6943

// socket constants
#define SOCKET_DOMAIN    AF_INET
#define SOCKET_TYPE      SOCK_STREAM
#define SOCKET_PROTOCOL  IPPROTO_TCP


/**
    Create a socket and return its sockfd
*/
int make_socket()
{
    // create socket FD
    int sockfd = socket(
        SOCKET_DOMAIN,
        SOCKET_TYPE,
        SOCKET_PROTOCOL
    );

    // set TCP_NODELAY to 1 (true)
    int no_delay = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &no_delay, sizeof(no_delay));

    return sockfd;
}


/**
    Attempt to bind server socket to port
*/
int bind_server_socket(int sockfd)
{
    // allow port reuse
    int reuse = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(reuse));

    // create sockaddr
    struct sockaddr_in server_address;
    bzero((char *) &server_address, sizeof(server_address));

    // populate sockaddr
    server_address.sin_family      = SOCKET_DOMAIN;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port        = ntohs(PORT_NUMBER);

    // attempt to bind. directly returns bind() result, to be handled later
    return bind(
        sockfd,
        (struct sockaddr *) &server_address,
        sizeof(server_address)
    );
}


/**
    Given struct hostent * (gotten by calling gethostbyname(HOSTNAME)), builds a socket
    address.
*/
struct sockaddr_in get_server_address(struct hostent *server)
{
    // the INET sockaddr we will be returning
    struct sockaddr_in result;
    bzero((char *) &result, sizeof(result));
   
    // copy hostent info into sockaddr
    result.sin_family = SOCKET_DOMAIN;
    bcopy((char *) server->h_addr, (char *) &result.sin_addr.s_addr, server->h_length);
    result.sin_port = htons(PORT_NUMBER);
    
    return result;
}
