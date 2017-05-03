/**

    header go here?

*/

#pragma once


// errors
#define IRCDATA_CONNREFUSED  -1
#define IRCDATA_UNSUPPORTED  -2


// data
#define IRCDATA_LEAVE  0
#define IRCDATA_JOIN   1
#define IRCDATA_MSG    2
#define IRCDATA_FILE   3


// constraints
#define IRCDATA_MAXLEN  256


struct ircdata_t
{
    int type;
    
    char *contents;
    int contents_length;
};


/** Helper method: create struct ircdata_t with the given info */
struct ircdata_t ircdata_create(int type, char *contents, int contents_length)
{
    struct ircdata_t result;

    result.type            = type;
    result.contents        = contents;
    result.contents_length = contents_length;

    return result;
}
