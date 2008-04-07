#!/bin/sh

insmod test-mark-speed-empty.ko
for a in `seq 1 10`; do cat /proc/testmark;done
rmmod test-mark-speed-empty
RESEMP=`dmesg |tail -n 10 |sed 's/^\[.*\] cycles : \(.*\)$/\1/'`

insmod test-mark-speed.ko
for a in `seq 1 10`; do cat /proc/testmark;done
rmmod test-mark-speed
RESSTD=`dmesg |tail -n 10 |sed 's/^\[.*\] cycles : \(.*\)$/\1/'`

insmod test-mark-speed-opt.ko
for a in `seq 1 10`; do cat /proc/testmark;done
rmmod test-mark-speed-opt
RESOPT=`dmesg |tail -n 10|sed 's/^\[.*\] cycles : \(.*\)$/\1/'`

insmod test-mark-speed-edit.ko
for a in `seq 1 10`; do cat /proc/testmark;done
rmmod test-mark-speed-edit
RESNOP=`dmesg |tail -n 10|sed 's/^\[.*\] cycles : \(.*\)$/\1/'`

echo "20000 iterations"

echo "Numbers for empty loop"

SUM="0"
for a in $RESEMP; do SUM=$[$SUM + $a]; done
RESEMP=$[$SUM / 10]

echo $RESEMP

echo "Numbers for normal marker"

SUM="0"
for a in $RESSTD; do SUM=$[$SUM + $a]; done
RESSTD=$[$SUM / 10]

echo $RESSTD

echo "Numbers for optimized marker"
SUM="0"
for a in $RESOPT; do SUM=$[$SUM + $a]; done
RESOPT=$[$SUM / 10]
echo $RESOPT

echo "Numbers for NOP replacement of function call"
SUM="0"
for a in $RESNOP; do SUM=$[$SUM + $a]; done
RESNOP=$[$SUM / 10]
echo $RESNOP

