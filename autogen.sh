#!/bin/sh

# Run this to generate a new configure script

ac=`which autoconf 2>/dev/null`
test -z "$ac" && ac=/usr/local/gnu-autotools/bin/autoconf
if [ -x "$ac" ]; then
    "$ac" && echo "Finished! Now run configure. If in doubt run ./configure --help"
else
    echo "Please install Gnu autoconf to build from CVS." >&2
    exit 1
fi
