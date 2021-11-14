#!/bin/sh

ioval=$1
cd /sys/class/gpio
echo "67" > export
echo "out" > gpio67/direction 
echo "$ioval" > gpio67/value
cd -
