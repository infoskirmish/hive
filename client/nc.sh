#!/bin/bash

# for LINUX or SOLARIS
# shell script to recursive listen for beacons
# useful for testing while beacons are plaintext

CMD="nc -v -l -p 9999"

# some versions of linux prefer the port w/out the -p option
#CMD="nc -v -l 9999"

while [ 1 ]
do
echo $CMD
$CMD
date
sleep 2
echo ""
done
