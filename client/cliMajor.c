/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           cliMajor.c
    Description:    Client code
    
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <pthread.h>

#include <errno.h>
#include <signal.h>


#include "ircdata.h"
#include "sockets.h"


// buffer input from stdin
#define INPUT_BUFFER_LENGTH  2000


char buf_input[INPUT_BUFFER_LENGTH];  // stdin input buffer
char username[USERNAME_MAXLEN + 1];   // client username

struct ircdata_t outgoing_data;       // should be reused for all outgoing data, in send_data()
FILE *outfile;                        // client-side copy of file received from server

int running = 1,       // are we running?
    getting_file = 0,  // are we in the process of receiving a file?
    sockfd;            // server sockfd


// forward declaration: safe exit (closes (sock)?fds, etc)
int safe_exit(int);


// send a new packet to the server. fills in username automatically.
int send_data(int type, char *contents)
{
    if (!sockfd) return -1;

    // zero-out outgoing_data, to be re-used
    bzero(&outgoing_data, sizeof(outgoing_data));

    // re-create the outgoing ircdata_t and copy in username from global
    outgoing_data = ircdata_create(type, contents);
    strncpy(outgoing_data.username, username, USERNAME_MAXLEN + 1);

    // return value: result of write()
    int result = write(sockfd, &outgoing_data, sizeof(outgoing_data));

    // handle error: server closed connection before write
    if (result == 0)
    {
        printf("Server closed the connection\n");
        return safe_exit(0);
    }

    // handle error: unable to write
    if (result < 0)
    {
        printf("Error writing to server (%d)\n", result);
        return safe_exit(result);
    }

    // return result of write()
    return result;
}


// send a file payload to the server
void send_file(char *ifname)
{
    int nread;  // bytes read from file

    char *buff = malloc(IRCDATA_FILE_BUFLEN);  // input file buffer
    FILE *in_f = fopen (ifname,"rb");          // read local file

    // handle error: input file failed to open
    if (!in_f)
    {
	printf("Error opening file '%s'. Does it exist?\n", ifname);
        return;
    }

    // while we can read data from the file, send it to the server
    while ((nread = fread(buff, 1, IRCDATA_FILE_BUFLEN, in_f)) > 0)
    {
        // send data directly to server
        send_data(IRCDATA_FILE, buff);

	if (nread < IRCDATA_FILE_BUFLEN) break;
    }
    
    
    fclose(in_f);  // !!! close file
    free(buff);    // !!! free buffer

    // tell the server that we're done sending the file
    send_data(IRCDATA_FILE_DONE, "");
}


// safely exit. close all fds/sockfds, free malloc()'d memory, etc.
int safe_exit(int code)
{
    running = 0;

    // close sockfd if it's open
    if (sockfd)
    {
        send_data(IRCDATA_LEAVE, "Goodbye!");
        close(sockfd);
    }

    // zero-out username and stdin input buffer
    // (these are on the stack, so no need to free()

    bzero(username, USERNAME_MAXLEN + 1);
    bzero(buf_input, INPUT_BUFFER_LENGTH);

    printf("\n");  // trailing newlines make everything look nice
    exit(code);

    return code;   // !!! yes, this is dead code, but GCC won't compile without it (-Werror -Wall)
}


// listen for messages from the server
void *message_listener_worker(void *arg)
{
    struct ircdata_t incoming;  // re-use whenever possible
    int bytes;  // num bytes read from server

    // first things first: join the server
    send_data(IRCDATA_JOIN, "");

    while (running && (bytes = read(sockfd, &incoming, sizeof(incoming))) > 0)
    {
        running = 1;  // reset on every loop entry

        switch (incoming.type)
        {
            // bugfix
            case 0:
                break;

            // user joins the server
            case IRCDATA_JOIN:
            {
                printf("*** %s joined the server\n", incoming.username);
                break;
            }

            // user leaves the server
            case IRCDATA_LEAVE:
            {
                if (strcmp(incoming.username, "") == 0) break;

                printf("*** %s left the server\n", incoming.username);
                break;
            }

            // user sends a message to the server
            case IRCDATA_MSG:
            {
                // username goes in brackets, e.g. <user>
                char *username_bracketed = malloc(USERNAME_MAXLEN + 2);
                sprintf(username_bracketed, "<%s>", incoming.username);

                printf("%s  %s\n", username_bracketed, incoming.contents);

                free(username_bracketed);

                break;
            }

            // received file data from server
            case IRCDATA_FILE:
            {
                if (!getting_file || !outfile)
                {
                    getting_file = 1;
                    outfile = fopen(incoming.filename, "wb");
                }

                fwrite(incoming.contents, 1, incoming.contents_length, outfile);

                break;
            }

            // done receiving file data from server
            case IRCDATA_FILE_DONE:
            {
                getting_file = 0;

                if (outfile)
                    fclose(outfile);

                printf("::: New file from %s: '%s'\n", incoming.username, incoming.filename);

                break;
            }

            // server received file OK
            case IRCDATA_FILE_OK:
            {
                // file received OK by server
                printf("::: Sent file '%s' to server\n", incoming.filename);
                break;
            }
    
            // handle errors and unknown messages
            default:
                // server errors are negative ints, and "contents" always has a relevant error message
                if (incoming.type < 0) 
                {
                    printf("*** Server error: %s\n", incoming.contents);
                    running = 0;
                    safe_exit(0);
                }

                // anything else is unknown. ignore it.
                break;
        }
    }
   
    // handle error: unable to read from server 
    if (bytes < 0)
        printf("*** Error reading from server\n");

    // handle error: connection closed by server
    if (bytes == 0)
        printf("*** Connection closed by server\n");

    // no longer running
    running = 0;

    // exit
    safe_exit(1);
    
    return NULL;  // !!! again, dead code, but GCC won't compile without it (-Werror -Wall)
}


// handle SIGINT (^C)
void on_sigint(int signal) { safe_exit(signal); }



// MAIN EXECUTING CODE BELOW



/** main executing function */
int main()
{
    // trap SIGINT for safe exit
    signal(SIGINT, &on_sigint);



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


    printf("Connecting to server %s as %s... ", HOSTNAME, username);

    // make socket with default values
    sockfd = make_socket();
    
    // lookup hostname
    struct hostent *server = gethostbyname(HOSTNAME);
    if (server == NULL)
    {
        printf("\n\nUnable to lookup host: %s\n", HOSTNAME);
        safe_exit(-1);
    }

    // build server address from hostname info (sockets.h)
    struct sockaddr_in server_address = get_server_address(server);
    

    // attempt to connect    
    if (connect(sockfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        printf("\n\nUnable to connect: %s\n", strerror(errno));
        safe_exit(1);
    }


    // connected. spawn message listener thread.
    pthread_t listener_thread;
    int thread_create = pthread_create(&listener_thread, NULL, message_listener_worker, NULL);

    // handle error: unable to create listener thread    
    if (thread_create != 0)
    {
        printf("Unable to spawn listener thread! Exiting.\n");
        safe_exit(thread_create);
    }

    // we're connected!
    printf("connected.\n\nCommands: message, put, quit\n\n");



    /* time for command input */


    int run= 1;
    while (running && (fgets(buf_input, INPUT_BUFFER_LENGTH, stdin) != NULL))
    {
        // grab the command off the front of the input
        char *command = strtok(buf_input, " ");

        // handle message
        if (strncmp(command, "message", strlen("message")) == 0)
        {
            char *text = strtok(NULL, "\n");

            if (text == NULL)
                printf("*** ERROR: message is missing. Usage message [user message]\n");

            else
                send_data(IRCDATA_MSG, text);
        }

        // handle quit
        else if (strncmp(command, "quit", strlen("quit")) == 0)
        {
	    break;
        }
	
        // handle put
	else if (strncmp(command, "put", strlen("put")) == 0)
	{
            // grab filename off the end of the input
	    char *fname = strtok(NULL, "\n");

            // handle error: no filename given
	    if (fname == NULL)
		printf("*** ERROR: file name is missing. Usage file [file name]\n");

            // handle error: filename too long
            else if (strlen(fname) > FILENAME_MAXLEN)
                printf("*** ERROR: file name is too long. Max is %d chars.\n", FILENAME_MAXLEN);
        
            // looks OK. send a file initiation packet (so server knows to expect one) and send_file()
	    else
            {
		send_data(IRCDATA_FILE, fname);
	    	send_file(fname);	
            }
	}

        // handle error: unknown command
	else 
	{   
	    if (run == 1) run = 0;
	    else printf("*** ERROR: unknown command '%s'. Commands: message, put, quit\n", strtok(command, "\n"));
	
    	}
    }

    // we've broken out of the input loop somehow. safely exit.
    return safe_exit(0);
}
