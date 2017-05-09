#!make

# makefile
# oh my god i hate makefiles.

CC        = gcc
CFLAGS    = -g -Werror -Wall -I$(LIBSDIR)

LIBS      = -pthread
LIBSDIR   = ./lib

OUTDIR    = ./bin

CLIENTDIR = ./client
SERVERDIR = ./server

DEPLOYDIR = ./submit


both:
	make client
	make server


.PHONY: client server clean deploy

cliMajor.c: $(LIBSDIR)/ircdata.h
srvMajor.c: $(LIBSDIR)/ircdata.h


client: cliMajor.c
	@if [ ! -d "$(OUTDIR)" ]; then mkdir $(OUTDIR); fi
	@$(CC) $(CLIENTDIR)/cliMajor.c -o $(OUTDIR)/cliMajor $(LIBS) $(CFLAGS)


server: srvMajor.c
	@if [ ! -d "$(OUTDIR)" ]; then mkdir $(OUTDIR); fi
	@$(CC) $(SERVERDIR)/srvMajor.c -o $(OUTDIR)/srvMajor $(LIBS) $(CFLAGS)



clean:
	rm $(OUTDIR)/cliMajor
	rm $(OUTDIR)/srvMajor


deploy:
	@# find all files in $(DEPLOYDIR) not called 'makefile' and remove them
	find $(DEPLOYDIR) ! -name 'makefile' -type f -exec rm -f {} +
	@./deploy.sh
