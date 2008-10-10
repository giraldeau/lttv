#excluding locking
#excluding core markers, not connected to default.
echo Disconnecting all markers
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -v ^core_|grep -v ^locking_|grep -v ^lockdep_|grep -v ^lockdep|grep -v ^tap_`
for a in $MARKERS; do echo Disconnecting $a; echo "disconnect $a" > /proc/ltt; done

# Markers starting with "tap_" are considered high-speed.
echo Disconnecting high-rate markers to tap
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u |grep ^tap_`

#Uncomment the following to also stop recording lockdep events.
#MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -e ^tap_ -e ^lockdep`

for a in $MARKERS; do
	echo Disconnecting $a

	echo "disconnect $a ltt_tap_marker" > /proc/ltt
done
