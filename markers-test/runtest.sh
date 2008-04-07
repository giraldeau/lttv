#!/bin/sh

ITER=10
LOOPS=2000

insmod test-mark-speed-empty.ko
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-empty
RESEMP=`dmesg |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed.ko
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed
RESSTD=`dmesg |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-opt.ko
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-opt
RESOPT=`dmesg |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-edit.ko
#Patch with nops
cat /proc/testmark

for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-edit
RESNOP=`dmesg |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

echo "Results in cycles per loop"

echo "Cycles for wbinvd() loop (will be substracted from following results)"

SUM="0"
for a in $RESEMP; do SUM=$[$SUM + $a]; done
RESEMP=`echo $SUM/$ITER/$LOOPS | bc -l /dev/stdin`

echo $RESEMP

echo "Added cycles for normal marker"

SUM="0"
for a in $RESSTD; do SUM=$[$SUM + $a]; done
RESSTD=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`

echo $RESSTD

echo "Added cycles for optimized marker"
SUM="0"
for a in $RESOPT; do SUM=$[$SUM + $a]; done
RESOPT=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo $RESOPT

echo "Added cycles for NOP replacement of function call"
SUM="0"
for a in $RESNOP; do SUM=$[$SUM + $a]; done
RESNOP=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo $RESNOP

