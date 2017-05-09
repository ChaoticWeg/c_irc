#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>  // trap SIGINT for safe exit
#include <string.h>  // bzero()
#include <errno.h>   // strerror(), errno

#include <pthread.h>

#include "sockets.h" // socket utilities
#include "ircdata.h"
#include "client.h"


#define MAX_CLIENTS  4


/** GLOBALS
    must be closed/freed by safe_exit */


struct client_t clients[MAX_CLIENTS];

pthread_mutex_t client_mutex;


int sockfd_server = -1;  // server sockfd. must be closed on safe exit
int running       = 0;   // boolean: are we running? used by looping threads



/** safely exit by closing all FDs and freeing all memory */
int safe_exit(int code)
{
    printf("\nClosing server...\n");

    if (sockfd_server >= 0)
        close(sockfd_server);

    exit(code);
    return code; // dead code, but GCC won't compile without a return statement
}


/** SIGINT handler */
void on_sigint(int signal) { safe_exit(signal); }



void broadcast(struct ircdata_t outgoing)
{
    pthread_mutex_lock(&client_mutex);

        int i; for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].active == CLIENT_INACTIVE) continue;

            int sockfd    = clients[i].sockfd;
            int try_write = write(sockfd, &outgoing, sizeof(outgoing));

            if (try_write == 0)
            {
                printf("zero-byte response to broadcast() from client %d (sockfd %d). closing.\n", i, sockfd);
                clients[i].active = CLIENT_INACTIVE;
                close(clients[i].sockfd);
            }

            if (try_write < 0)
            {
                printf("unable to write broadcast() to client %d (sockfd %d). closing.\n", i, sockfd);
                clients[i].active = CLIENT_INACTIVE;
                close(clients[i].sockfd);
            }
        }

    pthread_mutex_unlock(&client_mutex);
}
void s_file(char *username,char *contents)
{
    pthread_mutex_lock(&client_mutex);
    printf("inloop: %s\n",contents);    
    int n;
    char buffer[256];
    FILE *in_f;

    in_f = fopen (contents, "w");
    while ((n = read(sockfd_server,buffer, 256)) > 0)
    {
	fwrite(buffer,1,n,in_f);
    }    

    pthread_mutex_unlock(&client_mutex);
}
int get_next_client_index(int active)
{
    int result = -1;

    pthread_mutex_lock(&client_mutex);

        int i; for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].active == active)
            {
                result = i;
                break;
            }
        }

    pthread_mutex_unlock(&client_mutex);
    
    // if no client's in/active status matches the arg
    return result;
}




// Thread worker functions

/** Client worker function. Parameter should be an int* referencing the current index */
void * client_worker(void *arg)
{
    int index; memcpy(&index, (int *) arg, sizeof(int));  // copy index value, in case it changes later (it shouldn't)

    // lock mutex and mark client as active
    pthread_mutex_lock(&client_mutex);
        clients[index].active = CLIENT_ACTIVE;
    pthread_mutex_unlock(&client_mutex);

    int sockfd = clients[index].sockfd;  // socket fd for this client

    struct ircdata_t incoming;  // incoming data
    struct ircdata_t outgoing;  // outgoing data. should rarely be used.

    int try_read,       // return value of read(), handled later if error or zero
        client_ok = 1;  // is this client ok? should we keep listening to this client?


    /** Loop and read from client, as long as client is "ok" and the packets are valid */

    while (client_ok && (try_read = read(sockfd, &incoming, sizeof(incoming))) > 0)
    {
        outgoing = ircdata_create(incoming.type, incoming.contents);
        strncpy(outgoing.username, incoming.username, USERNAME_MAXLEN);

        switch(incoming.type)
        {
            case IRCDATA_JOIN:
            {
                printf(">>> JOIN: %s\n", outgoing.username);
                broadcast(outgoing);

                break;
            }

            case IRCDATA_MSG:
            {
                printf(">>> MSG %s: %s\n", outgoing.username, outgoing.contents);
                broadcast(outgoing);

                break;
            }

            case IRCDATA_LEAVE:
            {
                printf(">>> LEAVE: %s\n", outgoing.username);
                broadcast(outgoing);
                
                break;
            }

            case IRCDATA_FILE:
            {
                printf(">>> FILE %s: %s\n", incoming.username, incoming.contents);
		printf("%s\n",incoming.contents);
		s_file(incoming.username,incoming.contents);
                break;
            }

            default:
                printf("Unknown packet type %d from client %d. Shunning...\n", incoming.type, index);
                break;
        }
    }


    // if we get down to here, the read either failed or was zero. handle each case.

    if (try_read < 0)
        printf("Error reading from client %d (sockfd: %d)\n", index, sockfd);

    if (try_read == 0)
        printf("Client %d disconnected\n", index);


    // lock mutex and set client status to inactive
    pthread_mutex_lock(&client_mutex);
        clients[index].active = CLIENT_INACTIVE;
    pthread_mutex_unlock(&client_mutex);

    // close sockfd
    close(sockfd);

    return NULL;
}


/** Server worker function. Listens for and accepts connections on the server sockfd. Dummy parameter. */
void server_worker(void *arg)
{
    printf("Listening on port %d\n\n", PORT_NUMBER);

    struct ircdata_t handshake_response; // outgoing ircdata_t. should only be used to respond to handshakes.
    // be sure to wash after each handshake.

    // we are listening and can accept() connections.
    while (running)
    {
        bzero(&handshake_response, sizeof(handshake_response));
        struct sockaddr_in client_address;
        socklen_t client_length;
        
        int sockfd_client = accept(sockfd_server, (struct sockaddr *) &client_address, &client_length);

        int next_active_client_index = get_next_client_index(CLIENT_INACTIVE);
        
        if (next_active_client_index < 0)
        {
            // no inactive client spot available

            handshake_response = ircdata_create(IRCDATA_CONNREFUSED, "Server is full");

            write(sockfd_client, &handshake_response, sizeof(handshake_response));
            
            close(sockfd_client);
            continue;
        }


        // we have an available client slot. lock the client mutex, assign the sockfd to the client, and
        // create the pthread.
        
        pthread_mutex_lock(&client_mutex);

            clients[next_active_client_index].sockfd = sockfd_client;
            int res = pthread_create(&(clients[next_active_client_index].thread_id), NULL,
                (void *) client_worker, &next_active_client_index);

            if (res != 0)
            {
                pthread_mutex_unlock(&client_mutex);
                
                handshake_response = ircdata_create(IRCDATA_INTERNALERR, "Unable to create thread");

                write(sockfd_client, &handshake_response, sizeof(handshake_response));

                close(sockfd_client);
                continue;
            }

        pthread_mutex_unlock(&client_mutex);

    }
}








/** main executing function */
int main()
{
    // mark running boolean = TRUE
    running = 1;

    // trap SIGINT for safe exit
    signal(SIGINT, &on_sigint);

    // initialize all client structs (in its own scope)
    {
        int i; for (i = 0; i < MAX_CLIENTS; i++)
        {
            clients[i].active = CLIENT_INACTIVE;
            // TODO anything else?
        }
    }
    
    pthread_mutex_init(&client_mutex, NULL);


    // create socket 
    sockfd_server = make_socket();
    if (sockfd_server < 0)
    {
        printf("Unable to create socket: %s\n", strerror(errno));
        safe_exit(-1);
    }

    // attempt to bind socket to name/port
    if (bind_server_socket(sockfd_server) != 0)
    {
        printf("Unable to bind server socket to port %d: %s\n", PORT_NUMBER, strerror(errno));
        safe_exit(-1);
    }


    // by now, the socket is bound to the port. now, listen.
    if (listen(sockfd_server, 4) != 0)
    {
        printf("Unable to listen on port %d: %s\n", PORT_NUMBER, strerror(errno));
        safe_exit(-1);
    }

    
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, (void *) &server_worker, &sockfd_server);

    pthread_join(listener_thread, NULL);
    

    return safe_exit(0);
}
