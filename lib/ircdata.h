/**

    header go here?

*/

#pragma once

#include <string.h> // bzero()


// errors
#define IRCDATA_CONNREFUSED  -1
#define IRCDATA_UNSUPPORTED  -2
#define IRCDATA_INTERNALERR  -3


// data
#define IRCDATA_LEAVE      0
#define IRCDATA_JOIN       1
#define IRCDATA_MSG        2
#define IRCDATA_FILE       3
#define IRCDATA_FILE_DONE  4


// constraints
#define IRCDATA_MAXLEN      2000
#define USERNAME_MAXLEN       15
#define IRCDATA_FILE_BUFLEN  256


/** ONLY assign char* members via strcpy */
struct ircdata_t
{
    int type;

    char username[USERNAME_MAXLEN + 1];
    
    int contents_length;
    char contents[IRCDATA_MAXLEN];
};


/** Helper method: create struct ircdata_t with the given info */
struct ircdata_t ircdata_create(int type, char *contents)
{
    struct ircdata_t result;

    result.type            = type;

    bzero(result.contents, IRCDATA_MAXLEN);
    strncpy(result.contents, contents, IRCDATA_MAXLEN);

    result.contents_length = strlen(result.contents) + 1;

    return result;
}
