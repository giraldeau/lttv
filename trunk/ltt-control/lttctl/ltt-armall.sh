#excluding core markers (already connected)
#excluding locking markers (high traffic)

DEBUGFSROOT=$(grep ^debugfs /proc/mounts | head -1 | awk '{print $2}')
MARKERSROOT=${DEBUGFSROOT}/ltt/markers

echo Connecting all markers

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
			echo Connecting ${m}
			echo 1 > ${m}/enable
		done
		;;
	esac
done


# Connect the interesting high-speed markers to the marker tap.
# Markers starting with "tap_" are considered high-speed.
#echo Connecting high-rate markers to tap
#MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u |grep ^tap_`
#
##Uncomment the following to also record lockdep events.
##MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -e ^tap_ -e ^lockdep`
#
#IFS=${N}
#for a in $MARKERS; do
#	echo Connecting $a
#
#	#redirect markers carrying state information to dedicated channels
#	case $a in
#	*)
#		CHANNEL=
#		;;
#	esac
#
#	echo "connect $a ltt_tap_marker" > /proc/ltt
#done
