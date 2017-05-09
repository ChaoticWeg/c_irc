/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           srvMajor.c
    Description:    Server code
    
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <signal.h>   // trap SIGINT for safe exit
#include <string.h>   // bzero()
#include <errno.h>    // strerror(), errno

#include <pthread.h>  // threading

#include "sockets.h"  // socket utilities
#include "ircdata.h"  // ircdata_t and helper methods
#include "client.h"   // active client structure


#define MAX_CLIENTS  4  // max number of active clients at once


//** GLOBALS
//    must be closed/freed by safe_exit 

struct client_t clients[MAX_CLIENTS];
pthread_mutex_t client_mutex;

int sockfd_server = -1;  // server sockfd. must be closed on safe exit
int running       = 0;   // boolean: are we running? used by looping threads

// end globals


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
void on_sigint(int signal) { safe_exit(0); }



/** broadcast the ircdata_t message to all clients EXCEPT the one to be ignored
    (if a negative int is passed to param 2 "ignore", then no clients are ignored) */
void broadcast_ignore(struct ircdata_t outgoing, int ignore)
{
    pthread_mutex_lock(&client_mutex);

        int i; for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].active == CLIENT_INACTIVE) continue;
            if (i >= 0 && i == ignore) continue;

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

/** helper method: broadcast to ALL clients */
void broadcast(struct ircdata_t outgoing) { broadcast_ignore(outgoing, -1); }


/** receive a file from */
void s_file(char *username, char *filename)
{
    pthread_mutex_lock(&client_mutex);

    int n;

    FILE *in_f;

    struct ircdata_t incoming;
    bzero((char *) &incoming, sizeof(incoming));

    in_f = fopen (filename, "wb");

    while ((n = read(sockfd_server, &incoming, sizeof(incoming))) > 0)
    {
        if (incoming.type == IRCDATA_FILE_DONE)
            break;

	fwrite(incoming.contents, 1, strlen(incoming.contents), in_f);
        bzero((char *) &incoming, sizeof(incoming));
    }

    fclose(in_f);

    pthread_mutex_unlock(&client_mutex);
}


/** get the index of the next available client slot. returns -1 if we are full. */
int get_next_client_index(int active)
{
    int result = -1;  // need this here so we can be sure the mutex is unlocked before returning
    
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
        // copy the contents of the incoming packet into the outgoing one
        // (since we'll most often need to just echo the event to clients)

        outgoing = ircdata_create(incoming.type, incoming.contents);
        strncpy(outgoing.username, incoming.username, USERNAME_MAXLEN);

        switch(incoming.type)
        {
            // bugfix
            case 0:
                break;

            // client joins
            case IRCDATA_JOIN:
            {
                printf(">>> JOIN: %s\n", outgoing.username);
                broadcast(outgoing);

                break;
            }

            // client sends a message to the server
            case IRCDATA_MSG:
            {
                printf(">>> MSG %s: %s\n", outgoing.username, outgoing.contents);
                broadcast(outgoing);

                break;
            }

            // client leaves
            case IRCDATA_LEAVE:
            {
                printf(">>> LEAVE: %s\n", outgoing.username);
                broadcast(outgoing);
                
                break;
            }

            // client sends a file
            case IRCDATA_FILE:
            {
                printf(">>> FILE %s: %s\n", incoming.username, incoming.contents);

                // length of filename
                int fname_length = strlen(incoming.contents) + 1;

                // temporarily store the filename
                char filename[fname_length];
                bzero(filename, fname_length);
                strncpy(filename, incoming.contents, fname_length);

                // open the file on the server side
                FILE *outfile = fopen(filename, "wb");
                if (!outfile)  // if the file cannot be opened
                {
                    printf("ERROR: unable to open file '%s'\n", filename);
                    outgoing = ircdata_create(IRCDATA_INTERNALERR, "Unable to open file");
                    write(sockfd, &outgoing, sizeof(outgoing));
                    break;
                }
               
                // re-use the "incoming" ircdata_t 
                bzero((char *) &incoming, sizeof(incoming));

                // read from the client socket until we get something other than an IRCDATA_FILE packet
                int nread;
                while ((nread = read(sockfd, &incoming, sizeof(incoming))) > 0)
                {
                    if (incoming.type != IRCDATA_FILE)
                        break;

                    // write the contents sent by the client
                    fwrite(incoming.contents, 1, strlen(incoming.contents) + 1, outfile);
                }

                // once we've received something that isn't an IRCDATA_FILE packet from the client, we're done
                // receiving the file

                printf("::: Done receiving file '%s' from %s\n", filename, incoming.username);
                fclose(outfile);  // !!! close the file

                
                // tell client we got the file ok (probably)

                struct ircdata_t bc = ircdata_create(IRCDATA_FILE_OK, "");
                ircdata_copy_filename(&bc, filename);
                write(sockfd, &bc, sizeof(bc));
                
                // re-open file to read and broadcast
                FILE *infile = fopen(filename, "rb");
                if (!infile)
                {
                    printf("ERROR: unable to re-open/broadcast file '%s'\n", filename);
                    break;
                }

                // create the buffer that fread() will read into
                char *read_buffer = malloc(IRCDATA_MAXLEN);
                bzero(read_buffer, IRCDATA_MAXLEN);

                // re-use bc from above
                bzero((char *) &bc, sizeof(bc));

                // read from infile, one chunk at a time
                while ((nread = fread(read_buffer, 1, IRCDATA_MAXLEN, infile)) > 0)
                {
                    // populate bc with info
                    bc.type = IRCDATA_FILE;
                    ircdata_copy_username(&bc, incoming.username);
                    ircdata_copy_filename(&bc, filename);
                    ircdata_copy_file_contents(&bc, read_buffer);

                    // broadcast the file chunk to all clients EXCEPT the sender
                    broadcast_ignore(bc, index);
                    
                    // zero-out the stuff, just in case
                    bzero(read_buffer, IRCDATA_MAXLEN);
                    bzero((char *) &bc, sizeof(bc));
                }


                // broadcast that we are done to everyone EXCEPT the sender
                bc.type = IRCDATA_FILE_DONE;
                ircdata_copy_filename(&bc, filename);
                ircdata_copy_username(&bc, incoming.username);
                broadcast_ignore(bc, index);

                printf("::: Done broadcasting file '%s' to clients\n", filename);
                fclose(infile); // !!! close the file
	        
		break;
	}
            default:
                // ignore any packet that has an unsupported packet type
                break;
        }
    }


    // if we get down to here, the read either failed or was zero. handle each case.

    if (try_read < 0)  // read failed with error code
        printf("Error reading from client %d (sockfd: %d)\n", index, sockfd);


    // lock mutex and set this client's status to inactive
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

        // client address info
        struct sockaddr_in client_address;
        socklen_t client_length;
       
        // accept a new client connection 
        int sockfd_client = accept(sockfd_server, (struct sockaddr *) &client_address, &client_length);

        // find the next client slot
        int next_active_client_index = get_next_client_index(CLIENT_INACTIVE);
        
        // this will only be negative if we are full, so if it's negative then we're full. reject the client.
        if (next_active_client_index < 0)
        {
            printf("!!! Client tried to connect, but we already have %d active connections.\n!!! Rejecting...\n", MAX_CLIENTS);

            // we have a packet type specifically for this! fun times!
            handshake_response = ircdata_create(IRCDATA_CONNREFUSED, "server is full");
            write(sockfd_client, &handshake_response, sizeof(handshake_response));
            
            // close the client socket and loop back, skipping what's below
            close(sockfd_client);
            continue;
        }


        // if we've reached here, then things are OK.
        // we have an available client slot. lock the client mutex, assign the sockfd to the client, and
        // create the pthread.
        
        pthread_mutex_lock(&client_mutex);  // clients[] critical section

            // assign this client's sockfd
            clients[next_active_client_index].sockfd = sockfd_client;

            // attempt to create this client's worker pthread
            int res = pthread_create(&(clients[next_active_client_index].thread_id), NULL,
                (void *) client_worker, &next_active_client_index);

            // handle failure code
            if (res != 0)
            {
                pthread_mutex_unlock(&client_mutex);  // unlock the mutex immediately if thread cannot be created
                
                // create "internal error" ircdata_t packet with thread creation error message and send it
                handshake_response = ircdata_create(IRCDATA_INTERNALERR, "Unable to create thread");
                write(sockfd_client, &handshake_response, sizeof(handshake_response));

                // close socket
                close(sockfd_client);
                continue;
            }

        pthread_mutex_unlock(&client_mutex);


        // the client worker thread will handle activating the client and any read/write ops directly from/to
        // the client. the server worker thread can loop back and accept another client connection.


    }
}



// MAIN EXECUTING CODE BELOW




/** main executing function */
int main()
{
    // mark running boolean = TRUE
    running = 1;

    // trap SIGINT for safe exit
    signal(SIGINT, &on_sigint);

    // initialize all client structs
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

    
    // create the thread that will listen for and accept new client connections.
    pthread_t listener_thread;
    pthread_create(&listener_thread, NULL, (void *) &server_worker, &sockfd_server);

    // join threads! then sleep until ^C.
    pthread_join(listener_thread, NULL);
    
    return safe_exit(0);  // just in case
}
