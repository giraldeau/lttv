#!/bin/sh
# LTTng patch creation 
# Creates a -all patch, and split it.
# Mathieu Desnoyers, october 2005
#$1 is the version

function wr () {
  
  cat $1 >> $2

}


NAME=patch-2.6.17-lttng-$1
ALL_NAME=$NAME-all.diff

rm -fr tmppatch
mkdir tmppatch

cd tmppatch

cp ../$ALL_NAME .

splitdiff -a -d $ALL_NAME


rm $ALL_NAME

for a in *; do
	cp $a $a.tmp;
	grep -v -e "^diff --git " -e "^new file mode " -e "^index " $a.tmp > $a
	rm $a.tmp;
done



FILE=../$NAME-debugfs.diff

IN="?_fs_debugfs_inode.c"

for a in $IN; do wr $a $FILE; done



FILE=../$NAME-instrumentation.diff

IN="?_fs_buffer.c
?_fs_compat.c
?_fs_exec.c
?_fs_ioctl.c
?_fs_open.c
?_fs_read_write.c
?_fs_select.c
?_ipc_msg.c
?_ipc_sem.c
?_ipc_shm.c
?_kernel_irq_handle.c
?_kernel_itimer.c
?_kernel_sched.c
?_kernel_signal.c
?_kernel_softirq.c
?_kernel_timer.c
?_kernel_module.c
?_mm_filemap.c
?_mm_memory.c
?_mm_page_alloc.c
?_mm_page_io.c
?_net_core_dev.c
?_net_ipv4_devinet.c
?_net_socket.c"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-arm.diff

IN="?_arch_arm_kernel_entry-common.S
?_arch_arm_kernel_calls.S
?_arch_arm_kernel_irq.c
?_arch_arm_kernel_process.c
?_arch_arm_kernel_sys_arm.c
?_arch_arm_kernel_time.c
?_arch_arm_kernel_traps.c"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-i386.diff

IN="?_arch_i386_kernel_entry.S
?_arch_i386_kernel_Makefile
?_arch_i386_kernel_ltt.c
?_arch_i386_kernel_syscall_table.S
?_arch_i386_kernel_process.c
?_arch_i386_kernel_sys_i386.c
?_arch_i386_kernel_traps.c
?_arch_i386_kernel_time.c
?_arch_i386_mm_fault.c
?_include_asm-i386_unistd.h
?_include_asm-i386_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-mips.diff

IN="?_arch_mips_kernel_irq.c
?_arch_mips_kernel_process.c
?_arch_mips_kernel_traps.c
?_arch_mips_kernel_unaligned.c
?_arch_mips_kernel_syscall.c
?_arch_mips_kernel_scall32-o32.S
?_arch_mips_kernel_scall64-64.S
?_arch_mips_kernel_scall64-n32.S
?_arch_mips_kernel_scall64-o32.S
?_arch_mips_mm_fault.c"
for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-ppc.diff
IN="?_arch_ppc_kernel_entry.S
?_arch_ppc_kernel_misc.S
?_arch_ppc_kernel_time.c
?_arch_ppc_kernel_traps.c
?_arch_ppc_mm_fault.c"
for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-powerpc.diff
IN="?_arch_powerpc_kernel_entry_32.S
?_arch_powerpc_kernel_entry_64.S
?_arch_powerpc_kernel_misc_32.S
?_arch_powerpc_kernel_misc_64.S
?_arch_powerpc_kernel_process.c
?_arch_powerpc_kernel_systbl.S
?_arch_powerpc_kernel_time.c
?_arch_powerpc_kernel_traps.c
?_arch_powerpc_mm_fault.c
?_arch_powerpc_kernel_irq.c
?_arch_powerpc_kernel_ppc_ksyms.c
?_arch_powerpc_kernel_syscalls.c
?_include_asm-powerpc_unistd.h"

for a in $IN; do wr $a $FILE; done

FILE=../$NAME-instrumentation-x86_64.diff

IN="?_arch_x86_64_ia32_ia32entry.S
?_arch_x86_64_ia32_ipc32.c
?_arch_x86_64_kernel_entry.S
?_arch_x86_64_kernel_ltt.c
?_arch_x86_64_kernel_Makefile
?_arch_x86_64_kernel_time.c
?_arch_x86_64_kernel_traps.c
?_arch_x86_64_mm_fault.c
?_include_asm-x86_64_ia32_unistd.h
?_include_asm-x86_64_unistd.h
?_include_asm-x86_64_system.h"

for a in $IN; do wr $a $FILE; done



FILE=../$NAME-instrumentation-s390.diff
IN="?_arch_s390_kernel_entry.S
?_arch_s390_kernel_sys_s390.c
?_arch_s390_kernel_traps.c
?_arch_s390_mm_fault.c"
for a in $IN; do wr $a $FILE; done


FILE=../$NAME-instrumentation-sh.diff
IN="?_arch_sh_kernel_irq.c
?_arch_sh_kernel_process.c
?_arch_sh_kernel_sys_sh.c
?_arch_sh_kernel_traps.c
?_arch_sh_mm_fault.c"
for a in $IN; do wr $a $FILE; done



FILE=../$NAME-facilities-headers.diff

IN="?_include_linux_ltt_ltt-facility-core.h
?_include_linux_ltt_ltt-facility-fs.h
?_include_linux_ltt_ltt-facility-fs_data.h
?_include_linux_ltt_ltt-facility-custom-fs_data.h
?_include_linux_ltt_ltt-facility-custom-stack.h
?_include_linux_ltt_ltt-facility-id-core.h
?_include_linux_ltt_ltt-facility-id-fs.h
?_include_linux_ltt_ltt-facility-id-fs_data.h
?_include_linux_ltt_ltt-facility-id-ipc.h
?_include_linux_ltt_ltt-facility-id-kernel.h
?_include_linux_ltt_ltt-facility-id-locking.h
?_include_linux_ltt_ltt-facility-id-memory.h
?_include_linux_ltt_ltt-facility-id-network.h
?_include_linux_ltt_ltt-facility-id-network_ip_interface.h
?_include_linux_ltt_ltt-facility-id-process.h
?_include_linux_ltt_ltt-facility-id-socket.h
?_include_linux_ltt_ltt-facility-id-stack.h
?_include_linux_ltt_ltt-facility-id-statedump.h
?_include_linux_ltt_ltt-facility-id-timer.h
?_include_linux_ltt_ltt-facility-ipc.h
?_include_linux_ltt_ltt-facility-kernel.h
?_include_linux_ltt_ltt-facility-locking.h
?_include_linux_ltt_ltt-facility-memory.h
?_include_linux_ltt_ltt-facility-network.h
?_include_linux_ltt_ltt-facility-network_ip_interface.h
?_include_linux_ltt_ltt-facility-process.h
?_include_linux_ltt_ltt-facility-socket.h
?_include_linux_ltt_ltt-facility-stack.h
?_include_linux_ltt_ltt-facility-statedump.h
?_include_linux_ltt_ltt-facility-timer.h
?_include_asm-i386_ltt_ltt-facility-id-kernel_arch_i386.h
?_include_asm-i386_ltt_ltt-facility-kernel_arch_i386.h
?_include_asm-i386_ltt_ltt-facility-custom-stack.h
?_include_asm-i386_ltt_ltt-facility-custom-locking.h
?_include_asm-arm_ltt_ltt-facility-id-kernel_arch_arm.h
?_include_asm-arm_ltt_ltt-facility-kernel_arch_arm.h
?_include_asm-mips_ltt_ltt-facility-id-kernel_arch_mips.h
?_include_asm-mips_ltt_ltt-facility-kernel_arch_mips.h
?_include_asm-powerpc_ltt_ltt-facility-id-kernel_arch_powerpc.h
?_include_asm-powerpc_ltt_ltt-facility-kernel_arch_powerpc.h
?_include_asm-ppc_ltt_ltt-facility-id-kernel_arch_ppc.h
?_include_asm-ppc_ltt_ltt-facility-kernel_arch_ppc.h
?_include_asm-x86_64_ltt_ltt-facility-id-kernel_arch_x86_64.h
?_include_asm-x86_64_ltt_ltt-facility-kernel_arch_x86_64.h
?_include_asm-x86_64_ltt_ltt-facility-custom-stack.h
?_include_asm-x86_64_ltt_ltt-facility-custom-locking.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-facilities-loader.diff

IN="?_ltt_Makefile
?_ltt_ltt-facility-loader-core.c
?_ltt_ltt-facility-loader-core.h
?_ltt_ltt-facility-loader-fs.c
?_ltt_ltt-facility-loader-fs.h
?_ltt_ltt-facility-loader-fs_data.c
?_ltt_ltt-facility-loader-fs_data.h
?_ltt_ltt-facility-loader-ipc.c
?_ltt_ltt-facility-loader-ipc.h
?_ltt_ltt-facility-loader-kernel.c
?_ltt_ltt-facility-loader-kernel.h
?_ltt_ltt-facility-loader-locking.c
?_ltt_ltt-facility-loader-locking.h
?_ltt_ltt-facility-loader-memory.c
?_ltt_ltt-facility-loader-memory.h
?_ltt_ltt-facility-loader-network.c
?_ltt_ltt-facility-loader-network.h
?_ltt_ltt-facility-loader-network_ip_interface.c
?_ltt_ltt-facility-loader-network_ip_interface.h
?_ltt_ltt-facility-loader-process.c
?_ltt_ltt-facility-loader-process.h
?_ltt_ltt-facility-loader-socket.c
?_ltt_ltt-facility-loader-socket.h
?_ltt_ltt-facility-loader-stack.c
?_ltt_ltt-facility-loader-stack.h
?_ltt_ltt-facility-loader-statedump.c
?_ltt_ltt-facility-loader-statedump.h
?_ltt_ltt-facility-loader-timer.c
?_ltt_ltt-facility-loader-timer.h
?_ltt_ltt-facility-loader-kernel_arch_i386.c
?_ltt_ltt-facility-loader-kernel_arch_i386.h
?_ltt_ltt-facility-loader-kernel_arch_arm.c
?_ltt_ltt-facility-loader-kernel_arch_arm.h
?_ltt_ltt-facility-loader-kernel_arch_mips.c
?_ltt_ltt-facility-loader-kernel_arch_mips.h
?_ltt_ltt-facility-loader-kernel_arch_powerpc.c
?_ltt_ltt-facility-loader-kernel_arch_powerpc.h
?_ltt_ltt-facility-loader-kernel_arch_ppc.c
?_ltt_ltt-facility-loader-kernel_arch_ppc.h
?_ltt_ltt-facility-loader-kernel_arch_x86_64.c
?_ltt_ltt-facility-loader-kernel_arch_x86_64.h"


for a in $IN; do wr $a $FILE; done

FILE=../$NAME-facilities.diff

IN="?_include_linux_ltt-facilities.h
?_kernel_ltt-facilities.c"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-build.diff

IN="?_Makefile"

for a in $IN; do wr $a $FILE; done

FILE=../$NAME-core.diff

IN="?_MAINTAINERS
?_include_asm-alpha_ltt.h
?_include_asm-arm26_ltt.h
?_include_asm-arm_ltt.h
?_include_asm-cris_ltt.h
?_include_asm-frv_ltt.h
?_include_asm-generic_ltt.h
?_include_asm-h8300_ltt.h
?_include_asm-i386_ltt.h
?_include_asm-ia64_ltt.h
?_include_asm-m32r_ltt.h
?_include_asm-m68k_ltt.h
?_include_asm-m68knommu_ltt.h
?_include_asm-mips_ltt.h
?_include_asm-mips_mipsregs.h
?_include_asm-mips_timex.h
?_arch_mips_kernel_time.c
?_include_asm-parisc_ltt.h
?_include_asm-powerpc_ltt.h
?_include_asm-ppc_ltt.h
?_include_asm-s390_ltt.h
?_include_asm-sh64_ltt.h
?_include_asm-sh_ltt.h
?_include_asm-sparc64_ltt.h
?_include_asm-sparc_ltt.h
?_include_asm-um_ltt.h
?_include_asm-v850_ltt.h
?_include_asm-x86_64_ltt.h
?_include_linux_ltt-core.h
?_include_linux_netlink.h
?_include_linux_sched.h
?_ltt_Kconfig
?_ltt_ltt-core.c
?_Documentation_ioctl-number.txt
?_ltt_ltt-relay.c
?_arch_i386_Kconfig
?_arch_ppc_Kconfig
?_arch_powerpc_Kconfig
?_arch_arm_Kconfig
?_arch_mips_Kconfig
?_arch_x86_64_Kconfig
?_init_main.c
?_kernel_Makefile
?_kernel_ltt-base.c
?_kernel_ltt-heartbeat.c
?_kernel_ltt-syscall.c
?_kernel_sys_ni.c
?_kernel_exit.c
?_kernel_fork.c"

for a in $IN; do wr $a $FILE; done

FILE=../$NAME-modules.diff

IN="?_ltt_ltt-control.c
?_ltt_ltt-control.h
?_ltt_ltt-statedump.c"

for a in $IN; do wr $a $FILE; done

cd ..

rm $ALL_NAME
tar cvfj $NAME.tar.bz2 $NAME-*

