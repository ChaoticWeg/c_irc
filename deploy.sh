#!/bin/bash

TLDIR=`pwd` # top-level directory: the directory that we are currently in
TARGETDIR="$TLDIR/submit" # target directory: directory that the files will be copied to

EXITCODE=0

do_copy() { echo "cp $1 $2"; cp $1 $2; }

# loop through source directories
for DIR in "$TLDIR/client" "$TLDIR/server" "$TLDIR/lib"
do

    # copy each source file
    for FILE in $DIR/*.c
    do
        BFILE=`basename $FILE`
        if [ "$BFILE" != "*.c" ]; then do_copy "$DIR/$BFILE" "$TARGETDIR/$BFILE"; fi
    done

    # copy each header file
    for FILE in $DIR/*.h
    do
        BFILE=`basename $FILE`
        if [ "$BFILE" != "*.h" ]; then do_copy "$DIR/$BFILE" "$TARGETDIR/$BFILE"; fi
    done

done
