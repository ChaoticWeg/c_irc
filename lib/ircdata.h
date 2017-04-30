/**

    header go here?

*/

#pragma once


#define IRCDATA_LEAVE  0
#define IRCDATA_JOIN   1
#define IRCDATA_MSG    2
#define IRCDATA_FILE   3

#define IRCDATA_MAXLEN  256


struct ircdata_t
{
    int type;
    
    char *contents;
    int contents_length;
};
