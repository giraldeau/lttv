#excluding locking
#excluding core markers, not connected to default.
echo Disconnecting all markers
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2 " " $4}'|sort -u|grep -v ^metadata|grep -v ^locking|grep -v ^lockdep|grep -v ^tap`

#separator is newline
IFS=$
for a in $MARKERS; do echo Disconnecting $a; echo "disconnect $a" > /proc/ltt; done

# Markers starting with "tap_" are considered high-speed.
echo Disconnecting high-rate markers to tap
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2 " " $4}'|sort -u |grep ^tap`

#Uncomment the following to also stop recording lockdep events.
#MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -e ^tap_ -e ^lockdep`

#separator is newline
IFS=$
for a in $MARKERS; do
	echo Disconnecting $a

	echo "disconnect $a ltt_tap_marker" > /proc/ltt
done
