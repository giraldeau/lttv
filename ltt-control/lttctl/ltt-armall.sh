#excluding core markers (already connected)
#excluding locking markers (high traffic)

#scheduler probe
echo Loading probes
modprobe -q ltt-sched

echo Connecting all markers
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -v ^core_|grep -v ^locking_ |grep -v ^internal_`

echo Connecting internal_kernel_sched_schedule
echo "connect internal_kernel_sched_schedule scheduler" > /proc/ltt

for a in $MARKERS; do echo Connecting $a; echo "connect $a default" > /proc/ltt; done
