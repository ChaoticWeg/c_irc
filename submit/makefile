#!make

CC        = gcc
CFLAGS    = -g -I$(LIBSDIR)

LIBS      = -pthread
LIBSDIR   = .

OUTDIR    = .

CLIENTDIR = .
SERVERDIR = .



both:
	@make -s client
	@make -s server


cliMajor.c: $(LIBSDIR)/ircdata.h
srvMajor.c: $(LIBSDIR)/ircdata.h


client: cliMajor.c
	@$(CC) $(CLIENTDIR)/cliMajor.c -o $(OUTDIR)/cliMajor $(LIBS) $(CFLAGS)


server: srvMajor.c
	@$(CC) $(SERVERDIR)/srvMajor.c -o $(OUTDIR)/srvMajor $(LIBS) $(CFLAGS)



clean:
	@rm $(OUTDIR)/cliMajor
	@rm $(OUTDIR)/srvMajor
