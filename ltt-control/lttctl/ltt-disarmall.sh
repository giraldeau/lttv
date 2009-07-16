#excluding core markers (already connected)
#excluding locking markers (high traffic)

DEBUGFSROOT=$(grep ^debugfs /proc/mounts | head -1 | awk '{print $2}')
MARKERSROOT=${DEBUGFSROOT}/ltt/markers

echo Disconnecting all markers

for c in ${MARKERSROOT}/*; do
	case ${c} in
	${MARKERSROOT}/metadata)
		;;
	${MARKERSROOT}/locking)
		;;
	${MARKERSROOT}/lockdep)
		;;
	*)
		for m in ${c}/*; do
			echo Disconnecting ${m}
			echo 0 > ${m}/enable
		done
		;;
	esac
done

## Markers starting with "tap_" are considered high-speed.
#echo Disconnecting high-rate markers to tap
#MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2 " " $4}'|sort -u |grep ^tap`
#
##Uncomment the following to also stop recording lockdep events.
##MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -e ^tap_ -e ^lockdep`
#
#IFS=${N}
#for a in $MARKERS; do
#	echo Disconnecting $a
#
#	echo "disconnect $a ltt_tap_marker" > /proc/ltt
#done
