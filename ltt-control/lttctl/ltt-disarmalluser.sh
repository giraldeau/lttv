#excluding core markers (already connected)
#excluding locking markers (high traffic)

echo Disconnecting all userspace markers of _RUNNING PROCESSES_ only !

for a in /proc/[0-9]*; do
	for marker in $a/markers; do
		echo Disonnecting marker $a:$marker
		case $marker in 
		*)
			CHANNEL=
			;;
		esac
		echo "disconnect $marker default dynamic $CHANNEL" > /proc/ltt
	done
done
