#!/bin/sh

# Regression Test Script for invoking specific targets in Makefiles
#
# 

USAGE="Usage: `basename $0` DIRECTORY TARGET"

if [ $# -lt 2 ] ; then
  echo "$USAGE"
  exit 1
fi

# this is the directory where the test will be performed
cd $1

# Create an start up script with the gpsim command
#   load STC
#
# The makefiles invoke gpsim with the command
#
# gpsim -i -c startup.stc -D STC = "actual.stc"
#
# the -D option creates a symbol named STC with the string
# value "actual.stc". The startup.stc file then symbolic
# references the target specific simulation script via
# the defined STC.


STARTUP_STC='startup.stc'

echo "load STC" > $STARTUP_STC
echo "run" >> $STARTUP_STC
echo "quit" >> $STARTUP_STC
echo "" >> $STARTUP_STC

LOGFILE='simresults.log'
touch $LOGFILE

# Run the simulation and save the results
make $2 >$LOGFILE

# Convention simulations will print a message containing the
# the word PASSED
grep "PASSED" $LOGFILE

if [ $? -ne 0 ] ; then
  echo "$1/make $2 -- regression test FAILED"
fi
