#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <pthread.h>

#include <errno.h>
#include <signal.h>


#include "ircdata.h"
#include "sockets.h"


#define INPUT_BUFFER_LENGTH  2000


char buf_input[INPUT_BUFFER_LENGTH];
char username[USERNAME_MAXLEN + 1];

struct ircdata_t outgoing_data;

int running = 1,
    sockfd;


int safe_exit(int);


int send_data(int type, char *contents)
{
    if (!sockfd) return -1;

    bzero(&outgoing_data, sizeof(outgoing_data));

    outgoing_data = ircdata_create(type, contents);
    strncpy(outgoing_data.username, username, USERNAME_MAXLEN + 1);

    int result = write(sockfd, &outgoing_data, sizeof(outgoing_data));

    if (result == 0)
    {
        printf("Server closed the connection\n");
        return safe_exit(0);
    }

    if (result < 0)
    {
        printf("Error writing to server (%d)\n", result);
        return safe_exit(result);
    }

    return result;
}


int safe_exit(int code)
{
    running = 0;

    if (sockfd)
    {
        send_data(IRCDATA_LEAVE, "Goodbye!");
        close(sockfd);
    }

    bzero(username, USERNAME_MAXLEN + 1);
    bzero(buf_input, INPUT_BUFFER_LENGTH);

    exit(code);
    return code; // dead code, but GCC won't compile without it
}


void *message_listener_worker(void *arg)
{
    struct ircdata_t incoming;

    int bytes;
    while (running && (bytes = read(sockfd, &incoming, sizeof(incoming))) > 0)
    {
        switch (incoming.type)
        {
            case IRCDATA_JOIN:
            {
                printf("*** %s joined the server\n", incoming.username);
                break;
            }

            case IRCDATA_LEAVE:
            {
                if (strcmp(incoming.username, "") == 0) break;

                printf("*** %s left the server\n", incoming.username);
                break;
            }

            case IRCDATA_MSG:
            {
                // username goes in brackets, e.g. <user>
                char *username_bracketed = malloc(USERNAME_MAXLEN + 2);
                sprintf(username_bracketed, "<%s>", incoming.username);

                // username should be padded so that all messages line up
                char *username_padded    = malloc(USERNAME_MAXLEN + 3);
                sprintf(username_padded, "%-*s", USERNAME_MAXLEN + 2, username_bracketed);

                printf("%s %s\n", username_padded, incoming.contents);

                break;
            }
    
            default:
                printf("*** Unknown/unsupported message received from server (%d): %s\n", incoming.type, incoming.contents);
                break;
        }
    }
    
    if (bytes < 0)
        printf("*** Error reading from server\n");

    if (bytes == 0)
        printf("*** Connection closed by server\n");

    running = 0;
    return NULL;
}


void on_sigint(int signal) { safe_exit(signal); }

int main()
{
    // trap SIGINT for safe exit
    signal(SIGINT, &on_sigint);

    // we're running!
    running = 1;


    /* time for username input */


    // actual username array should be one byte more than max length (null terminator)
    bzero(username, USERNAME_MAXLEN + 1);

    // clear buffer and prompt for username
    bzero(buf_input, INPUT_BUFFER_LENGTH);
    
    printf("Username: ");
    scanf("%s", buf_input);

    // copy input to username array and null-terminate it
    strncpy(username, strtok(buf_input, "\n"), USERNAME_MAXLEN);
    username[USERNAME_MAXLEN] = '\0';
   
    // zero-out the buffer, we're done with it for now 
    bzero(buf_input, INPUT_BUFFER_LENGTH);
    

    /* time to connect */

    printf("Connecting to server %s as %s...\n\n", HOSTNAME, username);

    // make socket with default values
    sockfd = make_socket();
    
    // lookup hostname
    struct hostent *server = gethostbyname(HOSTNAME);
    if (server == NULL)
    {
        printf("Unable to lookup host: %s\n", HOSTNAME);
        safe_exit(-1);
    }

    // build server address from hostname info (sockets.h)
    struct sockaddr_in server_address = get_server_address(server);
    

    // attempt to connect    
    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        printf("Unable to connect: %s\n", strerror(errno));
        safe_exit(1);
    }


    // connected. spawn message listener thread.
    pthread_t listener_thread;
    int thread_create = pthread_create(&listener_thread, NULL, message_listener_worker, NULL);
    
    if (thread_create != 0)
    {
        printf("Unable to spawn listener thread! Exiting.\n");
        safe_exit(thread_create);
    }

   
    // join server 
    send_data(IRCDATA_JOIN, "");


    while (running && (fgets(buf_input, INPUT_BUFFER_LENGTH, stdin) != NULL))
    {
        // handle this command
        char *command = strtok(buf_input, " ");
        if (strncmp(command, "message", strlen("message")) == 0)
        {
            char *text = strtok(NULL, "\n");
            if (text == NULL)
                printf("*** ERROR: unknown command %s. Commands: message, file, quit\n", command);

            else
                send_data(IRCDATA_MSG, text);
        }

        else if (strncmp(command, "quit", strlen("quit")) == 0)
        {
            send_data(IRCDATA_LEAVE, "");
            running = 0;
            return safe_exit(0);
        }
    }


    return safe_exit(0);
}
