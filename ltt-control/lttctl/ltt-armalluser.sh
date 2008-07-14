#excluding core markers (already connected)
#excluding locking markers (high traffic)

echo Connecting all userspace markers of _CURRENTLY RUNNING_ processes only !

for a in /proc/[0-9]*; do
	for marker in `cat $a/markers`; do
		echo Connecting marker $a:$marker
		case $marker in 
		*)
			CHANNEL=
			;;
		esac
		echo "connect $marker default dynamic $CHANNEL" > /proc/ltt
	done
done
