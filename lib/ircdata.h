/*
    
    CSCE 3600 Major Assignment
    
    Authors: Zach Newman (zrn0003)
             Shawn Lutch (sml0262)
    
    File:           ircdata.h
    Description:    Defines struct ircdata_t, the structure of the data sent between client and server
    
*/

#pragma once

#include <string.h> // bzero()


// errors
#define IRCDATA_CONNREFUSED  -1
#define IRCDATA_UNSUPPORTED  -2
#define IRCDATA_INTERNALERR  -3


// data
#define IRCDATA_LEAVE      1
#define IRCDATA_JOIN       2
#define IRCDATA_MSG        3
#define IRCDATA_FILE       4
#define IRCDATA_FILE_DONE  5
#define IRCDATA_FILE_OK    6


// constraints
#define IRCDATA_MAXLEN      2000
#define USERNAME_MAXLEN       15
#define FILENAME_MAXLEN       15
#define IRCDATA_FILE_BUFLEN  256


/** ONLY assign char* members via strcpy */
struct ircdata_t
{
    int type;

    char username[USERNAME_MAXLEN + 1];
    char filename[FILENAME_MAXLEN + 1];
    
    int contents_length;
    char contents[IRCDATA_MAXLEN + 1];
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

void ircdata_copy_file_contents(struct ircdata_t *result, char *contents)
{
    bzero(result->contents, IRCDATA_MAXLEN + 1);
    strncpy(result->contents, contents, IRCDATA_MAXLEN);
    result->contents_length = strlen(result->contents);
}

void ircdata_copy_username(struct ircdata_t *result, char *username)
{
    bzero(result->username, USERNAME_MAXLEN + 1);
    strncpy(result->username, username, USERNAME_MAXLEN);
    result->username[USERNAME_MAXLEN] = '\0';
}

void ircdata_copy_filename(struct ircdata_t *result, char *filename)
{
    bzero(result->filename, FILENAME_MAXLEN + 1);
    strncpy(result->filename, filename, FILENAME_MAXLEN);
    result->filename[FILENAME_MAXLEN] = '\0';
}
