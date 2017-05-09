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


CC        = gcc
CFLAGS    = -g -I$(LIBSDIR)

LIBS      = -pthread
LIBSDIR   = .

OUTDIR    = .

CLIENTDIR = .
SERVERDIR = .


.PHONY: client server clean all


all:
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
