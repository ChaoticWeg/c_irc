#!/bin/bash


OUTDIR=./bin  # no trailing slash!


TIMESTAMP=`TZ=America/Chicago date +"%D at %T"`

WHICH=$1
EXITCODE=0


# Validate input: must specify client or server
if ! [ "$WHICH" == "server" ] && ! [ "$WHICH" == "client" ]; then
    echo "Usage: $0 [client | server]"
    exit 1
fi

header() { clear; echo -ne "Testing major assignment $WHICH side\nStarted: $TIMESTAMP\n--\n\n"; }
footer() { echo -ne "\n--\nExited with code $EXITCODE\n\n"; exit $EXITCODE; }






build_side() {
    make $1
    EXITCODE=$?

    if [ "$EXITCODE" -ne "0" ]; then
        footer
    fi
}




header
build_side $WHICH

header

if [ "$WHICH" == "server" ]; then
    $OUTDIR/srvMajor
    EXITCODE=$?
elif [ "$WHICH" == "client" ]; then
    $OUTDIR/cliMajor
    EXITCODE=$?
fi

footer
