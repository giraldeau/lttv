#!/bin/sh

make clean
make #build no flush modules

ITER=10
LOOPS=20000

insmod test-mark-speed-empty.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-empty
RESEMP=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed
RESSTD=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-opt.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-opt
RESOPT=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-edit.ko
#Patch with nops
cat /proc/testmark

for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-edit
RESNOP=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-local.ko
#Patch with nops
cat /proc/testmark

for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-local
RESNOPLOCAL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`


make clean
make EXTRA_CFLAGS=-DCACHEFLUSH


insmod test-mark-speed-empty.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-empty
RESEMPFL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed
RESSTDFL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-opt.ko
cat /proc/testmark
for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-opt
RESOPTFL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-edit.ko
#Patch with nops
cat /proc/testmark

for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-edit
RESNOPFL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`

insmod test-mark-speed-local.ko
#Patch with nops
cat /proc/testmark

for a in `seq 1 $ITER`; do cat /proc/testmark;done
rmmod test-mark-speed-local
RESNOPLOCALFL=`dmesg |grep "cycles : " |tail -n 10 |sed 's/^\[.*\] //'| sed 's/cycles : \(.*\)$/\1/'`



echo "Results in cycles per loop"

echo "Cycles for empty loop (will be substracted for cached runs)"
SUM="0"
for a in $RESEMP; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESEMP=`echo $SUM/$ITER/$LOOPS | bc -l /dev/stdin`
echo $RESEMP

echo "Cycles for clflush() loop (will be substracted non-cached runs)"
SUM="0"
for a in $RESEMPFL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESEMPFL=`echo $SUM/$ITER/$LOOPS | bc -l /dev/stdin`
echo $RESEMPFL


echo -n "Added cycles for normal marker [cached, uncached]    "
SUM="0"
for a in $RESSTD; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESSTD=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo -n "[$RESSTD, "

SUM="0"
for a in $RESSTDFL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESSTDFL=`echo $SUM/$ITER/$LOOPS - $RESEMPFL | bc -l /dev/stdin`
echo "$RESSTDFL]"



echo -n "Added cycles for optimized marker [cached, uncached]    "
SUM="0"
for a in $RESOPT; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESOPT=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo -n "[$RESOPT, "

SUM="0"
for a in $RESOPTFL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESOPTFL=`echo $SUM/$ITER/$LOOPS - $RESEMPFL | bc -l /dev/stdin`
echo "$RESOPTFL]"


echo -n "Added cycles for NOP replacement of function call (1 pointer read, 5 local vars) [cached, uncached]    "
SUM="0"
for a in $RESNOP; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESNOP=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo -n "[$RESNOP, "

SUM="0"
for a in $RESNOPFL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESNOPFL=`echo $SUM/$ITER/$LOOPS - $RESEMPFL | bc -l /dev/stdin`
echo "$RESNOPFL]"


echo -n "Added cycles for NOP replacement of function call (6 local vars) [cached, uncached]    "
SUM="0"
for a in $RESNOPLOCAL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESNOPLOCAL=`echo $SUM/$ITER/$LOOPS - $RESEMP | bc -l /dev/stdin`
echo -n "[$RESNOPLOCAL, "

SUM="0"
for a in $RESNOPLOCALFL; do SUM=`echo "$SUM + $a" | bc -l /dev/stdin`; done
RESNOPLOCALFL=`echo $SUM/$ITER/$LOOPS - $RESEMPFL | bc -l /dev/stdin`
echo "$RESNOPLOCALFL]"

