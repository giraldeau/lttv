#excluding core markers (already connected)
#excluding locking markers (high traffic)

echo Disconnecting all userspace markers of _RUNNING PROCESSES_ only !

for a in /proc/[0-9]*; do
	for marker in `cat $a/markers | awk '{print $2}'`; do
		echo Disonnecting marker $a:$marker
		case $marker in 
		*)
			CHANNEL=
			;;
		esac
		echo "disconnect $marker default dynamic $CHANNEL" > /proc/ltt
	done
done
