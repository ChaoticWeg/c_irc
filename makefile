#!make

# makefile
# oh my god i hate makefiles.

CC =gcc
CFLAGS =-g -Werror -Wall -I$(LIBSDIR)

LIBS =-pthread
LIBSDIR =./lib

OUTDIR =./bin

CLIENTDIR =./client
SERVERDIR =./server



both:
	make client
	make server


cliMajor.c: $(LIBSDIR)/ircdata.h
srvMajor.c: $(LIBSDIR)/ircdata.h


client: cliMajor.c
	$(CC) $(CLIENTDIR)/cliMajor.c -o $(OUTDIR)/cliMajor $(LIBS) $(CFLAGS)


server: srvMajor.c
	$(CC) $(SERVERDIR)/srvMajor.c -o $(OUTDIR)/srvMajor $(LIBS) $(CFLAGS)



clean:
	rm $(OUTDIR)/cliMajor
	rm $(OUTDIR)/srvMajor
