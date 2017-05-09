#!make
#
#   
#   CSCE 3600 Major Assignment
#   
#   Authors: Zach Newman (zrn0003)
#            Shawn Lutch (sml0262)
#   
#   File:           makefile 
#   Description:    Defines GNU make rules for building the project.
#                   Adapted from the makefile used to test this project.
#


# GCC options
CC        = gcc
CFLAGS    = -g -I$(LIBSDIR)
CLIBS     = -pthread

# mutual library files
LIBSDIR   = .

# where to dump the binaries from GCC
OUTDIR    = .

# where client and server codes are kept
CLIENTDIR = .
SERVERDIR = .


# avoid rule names clashing with file/dir names
.PHONY: client server clean all


# make all the things
all:
	@make -s client
	@make -s server


# declare dependencies
cliMajor.c: $(LIBSDIR)/ircdata.h
srvMajor.c: $(LIBSDIR)/ircdata.h


# rule for building client
client: cliMajor.c
	@$(CC) $(CLIENTDIR)/cliMajor.c -o $(OUTDIR)/cliMajor $(CLIBS) $(CFLAGS)


# rule for building server
server: srvMajor.c
	@$(CC) $(SERVERDIR)/srvMajor.c -o $(OUTDIR)/srvMajor $(CLIBS) $(CFLAGS)


# rule for cleaning out binaries
clean:
	@rm $(OUTDIR)/cliMajor
	@rm $(OUTDIR)/srvMajor
