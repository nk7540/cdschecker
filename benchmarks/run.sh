#!/bin/sh
#
# Runs a simple test (default: ./barrier/barrier)
# Syntax:
#  ./run.sh [test program] [OPTIONS]
#  ./run.sh [OPTIONS]
#  ./run.sh [gdb [test program]]
#
# If you include a 'gdb' argument, the your program will be launched with gdb.
# You can also supply a test program argument to run something besides the
# default program.
#

# Get the directory in which this script is located
BINDIR="${0%/*}"

BIN=${BINDIR}/barrier/barrier
PREFIX=

export LD_LIBRARY_PATH=${BINDIR}/..
# export LD_LIBRARY_PATH=${BINDIR}/..:/usr/local/apache/apr-util/lib:/usr/local/apache/apr/lib
# For Mac OSX
export DYLD_LIBRARY_PATH=${BINDIR}/..

[ $# -gt 0 ] && [ "$1" = "gdb" ] && PREFIX=gdb && shift
[ $# -gt 0 ] && [ -e "$1" ] && BIN="$1" && shift

set -xe
$PREFIX $BIN $@
