#excluding core markers (already connected)
#excluding locking markers (high traffic)

echo Connecting all markers
MARKERS=`cat /proc/ltt|grep -v %k|awk '{print $2}'|sort -u|grep -v ^core_|grep -v ^locking_`

for a in $MARKERS; do
	echo Connecting $a

	#redirect markers carrying state information to dedicated channels
	case $a in
	list_process_state|user_generic_thread_brand|fs_exec|kernel_process_fork|kernel_process_free|kernel_process_exit|kernel_arch_kthread_create|list_statedump_end|list_vm_map)
		CHANNEL=processes
		;;
	list_interrupt|statedump_idt_table|statedump_sys_call_table)
		CHANNEL=interrupts
		;;
	list_network_ipv4_interface|list_network_ip_interface)
		CHANNEL=network
		;;
	kernel_module_load|kernel_module_free)
		CHANNEL=modules
		;;
	*)
		CHANNEL=
		;;
	esac

	echo "connect $a default dynamic $CHANNEL" > /proc/ltt
done
