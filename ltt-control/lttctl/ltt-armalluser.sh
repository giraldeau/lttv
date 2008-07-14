#excluding core markers (already connected)
#excluding locking markers (high traffic)

echo Connecting all userspace markers of _CURRENTLY RUNNING_ processes only !

for a in /proc/[0-9]*; do
	echo Connecting markers in $a

	for marker in $a/markers; do
		case $marker in 
		*)
			CHANNEL=
			;;
		esac
		echo "connect $marker default dynamic $CHANNEL" > /proc/ltt
	done
done
