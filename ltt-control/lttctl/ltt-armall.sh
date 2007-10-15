#excluding core markers (already connected)
#excluding locking markers (high traffic)
echo Connecting all markers
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -v ^core_|grep -v ^locking_`
for a in $MARKERS; do echo Connecting $a; echo "connect $a default" > /proc/ltt > /proc/ltt; done

#excluding locking markers (high traffic)
#core markers must be armed
echo Arming all markers
MARKERS=`cat /proc/ltt|awk '{print $2}'|sort -u|grep -v ^locking_`
for a in $MARKERS; do echo Arming $a; echo "arm $a" > /proc/ltt; done
