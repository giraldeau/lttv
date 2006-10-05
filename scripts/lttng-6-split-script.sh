#!/bin/sh
# LTTng patch creation 
# Creates a -all patch, and split it.
# Mathieu Desnoyers, october 2005
#$1 is the version

function wr () {
  
  cat $1 >> $2

}


PRENAME=patch
NAME=-2.6.18-lttng-$1
ALL_NAME=${PRENAME}${NAME}-all.diff
VALUE=1
printf -v COUNT "%02d" ${VALUE}

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

FILE=../${PRENAME}${COUNT}${NAME}-debugfs.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_fs_debugfs_inode.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-generic_vmlinux.lds.h
?_kernel_Kconfig.marker
?_include_linux_module.h
?_include_linux_marker.h
?_include_asm-arm_marker.h
?_include_asm-cris_marker.h
?_include_asm-frv_marker.h
?_include_asm-generic_marker.h
?_include_asm-h8300_marker.h
?_include_asm-i386_marker.h
?_include_asm-ia64_marker.h
?_include_asm-m32r_marker.h
?_include_asm-m68k_marker.h
?_include_asm-m68knommu_marker.h
?_include_asm-mips_marker.h
?_include_asm-parisc_marker.h
?_include_asm-powerpc_marker.h
?_include_asm-ppc64_marker.h
?_include_asm-ppc_marker.h
?_include_asm-s390_marker.h
?_include_asm-sh64_marker.h
?_include_asm-sh_marker.h
?_include_asm-sparc64_marker.h
?_include_asm-sparc_marker.h
?_include_asm-um_marker.h
?_include_asm-v850_marker.h
?_include_asm-x86_64_marker.h
?_include_asm-xtensa_marker.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-facility-core-headers.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_ltt_ltt-facility-core.h
?_include_ltt_ltt-facility-id-core.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-facility-loader-core.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_facilities_ltt-facility-loader-core.c
?_ltt_facilities_ltt-facility-loader-core.h
?_ltt_facilities_Makefile"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-facilities.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_ltt-facilities.h
?_ltt_ltt-facilities.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-timestamp.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-alpha_ltt.h
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
?_include_asm-xtensa_ltt.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-core-header.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_ltt-core.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-core.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_MAINTAINERS
?_ltt_ltt-core.c
?_ltt_ltt-heartbeat.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-tracer-header.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_ltt_ltt-tracer.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-tracer.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-tracer.c"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-transport.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-relay.c
?_Documentation_ioctl-number.txt"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-netlink-control.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-control.c
?_ltt_ltt-control.h
?_include_linux_netlink.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-userspace-tracing.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_sched.h
?_kernel_sys_ni.c
?_ltt_ltt-syscall.c
?_kernel_exit.c
?_kernel_fork.c
?_include_asm-i386_unistd.h
?_include_asm-powerpc_unistd.h
?_include_asm-x86_64_ia32_unistd.h
?_include_asm-x86_64_unistd.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-arm.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_arm_kernel_calls.S
?_arch_arm_kernel_entry-common.S
?_arch_arm_kernel_irq.c
?_arch_arm_kernel_process.c
?_arch_arm_kernel_ptrace.c
?_arch_arm_kernel_sys_arm.c
?_arch_arm_kernel_time.c
?_arch_arm_kernel_traps.c
?_include_asm-arm_irq.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_i386_kernel_process.c
?_arch_i386_kernel_ptrace.c
?_arch_i386_kernel_syscall_table.S
?_arch_i386_kernel_sys_i386.c
?_arch_i386_kernel_time.c
?_arch_i386_kernel_traps.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-mips.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_mips_kernel_irq.c
?_arch_mips_kernel_process.c
?_arch_mips_kernel_ptrace.c
?_arch_mips_kernel_scall32-o32.S
?_arch_mips_kernel_scall64-64.S
?_arch_mips_kernel_scall64-n32.S
?_arch_mips_kernel_scall64-o32.S
?_arch_mips_kernel_syscall.c
?_arch_mips_kernel_time.c
?_arch_mips_kernel_traps.c
?_arch_mips_kernel_unaligned.c
?_arch_mips_mm_fault.c
?_include_asm-mips_mipsregs.h"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_powerpc_kernel_entry_32.S
?_arch_powerpc_kernel_irq.c
?_arch_powerpc_kernel_misc_32.S
?_arch_powerpc_kernel_misc_64.S
?_arch_powerpc_kernel_ppc_ksyms.c
?_arch_powerpc_kernel_process.c
?_arch_powerpc_kernel_prom.c
?_arch_powerpc_kernel_ptrace.c
?_arch_powerpc_kernel_syscalls.c
?_arch_powerpc_kernel_systbl.S
?_arch_powerpc_kernel_time.c
?_arch_powerpc_kernel_traps.c
?_arch_powerpc_mm_fault.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-ppc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_ppc_Kconfig
?_arch_ppc_kernel_misc.S
?_arch_ppc_kernel_time.c
?_arch_ppc_kernel_traps.c
?_arch_ppc_mm_fault.c"


for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-x86_64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_x86_64_ia32_ia32entry.S
?_arch_x86_64_ia32_ipc32.c
?_arch_x86_64_kernel_entry.S
?_arch_x86_64_kernel_ptrace.c
?_arch_x86_64_kernel_time.c
?_arch_x86_64_kernel_traps.c
?_arch_x86_64_mm_fault.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

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
?_kernel_kthread.c
?_kernel_Makefile
?_kernel_lockdep.c
?_kernel_module.c
?_kernel_printk.c
?_kernel_sched.c
?_kernel_signal.c
?_kernel_softirq.c
?_kernel_timer.c
?_mm_filemap.c
?_mm_memory.c
?_mm_page_alloc.c
?_mm_page_io.c
?_net_core_dev.c
?_net_ipv4_devinet.c
?_net_socket.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-facilities-probes-headers.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_ltt_ltt-facility-custom-fs_data.h
?_include_ltt_ltt-facility-custom-stack_arch_i386.h
?_include_ltt_ltt-facility-custom-stack_arch_x86_64.h
?_include_ltt_ltt-facility-custom-stack.h
?_include_ltt_ltt-facility-fs_data.h
?_include_ltt_ltt-facility-fs.h
?_include_ltt_ltt-facility-id-fs_data.h
?_include_ltt_ltt-facility-id-fs.h
?_include_ltt_ltt-facility-id-ipc.h
?_include_ltt_ltt-facility-id-kernel_arch_arm.h
?_include_ltt_ltt-facility-id-kernel_arch_i386.h
?_include_ltt_ltt-facility-id-kernel_arch_mips.h
?_include_ltt_ltt-facility-id-kernel_arch_powerpc.h
?_include_ltt_ltt-facility-id-kernel_arch_ppc.h
?_include_ltt_ltt-facility-id-kernel_arch_x86_64.h
?_include_ltt_ltt-facility-id-kernel.h
?_include_ltt_ltt-facility-id-locking.h
?_include_ltt_ltt-facility-id-memory.h
?_include_ltt_ltt-facility-id-network.h
?_include_ltt_ltt-facility-id-network_ip_interface.h
?_include_ltt_ltt-facility-id-process.h
?_include_ltt_ltt-facility-id-socket.h
?_include_ltt_ltt-facility-id-stack_arch_i386.h
?_include_ltt_ltt-facility-id-stack.h
?_include_ltt_ltt-facility-id-statedump.h
?_include_ltt_ltt-facility-id-timer.h
?_include_ltt_ltt-facility-ipc.h
?_include_ltt_ltt-facility-kernel_arch_arm.h
?_include_ltt_ltt-facility-kernel_arch_i386.h
?_include_ltt_ltt-facility-kernel_arch_mips.h
?_include_ltt_ltt-facility-kernel_arch_powerpc.h
?_include_ltt_ltt-facility-kernel_arch_ppc.h
?_include_ltt_ltt-facility-kernel_arch_x86_64.h
?_include_ltt_ltt-facility-kernel.h
?_include_ltt_ltt-facility-locking.h
?_include_ltt_ltt-facility-memory.h
?_include_ltt_ltt-facility-network.h
?_include_ltt_ltt-facility-network_ip_interface.h
?_include_ltt_ltt-facility-process.h
?_include_ltt_ltt-facility-select-core.h
?_include_ltt_ltt-facility-select-default.h
?_include_ltt_ltt-facility-select-kernel.h
?_include_ltt_ltt-facility-select-network_ip_interface.h
?_include_ltt_ltt-facility-select-process.h
?_include_ltt_ltt-facility-select-statedump.h
?_include_ltt_ltt-facility-socket.h
?_include_ltt_ltt-facility-stack.h
?_include_ltt_ltt-facility-statedump.h
?_include_ltt_ltt-facility-timer.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-facilities-probes.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_facilities_ltt-facility-loader-fs.c
?_ltt_facilities_ltt-facility-loader-fs_data.c
?_ltt_facilities_ltt-facility-loader-fs_data.h
?_ltt_facilities_ltt-facility-loader-fs.h
?_ltt_facilities_ltt-facility-loader-ipc.c
?_ltt_facilities_ltt-facility-loader-ipc.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_arm.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_arm.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_i386.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_i386.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_mips.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_mips.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_powerpc.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_powerpc.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_ppc.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_ppc.h
?_ltt_facilities_ltt-facility-loader-kernel_arch_x86_64.c
?_ltt_facilities_ltt-facility-loader-kernel_arch_x86_64.h
?_ltt_facilities_ltt-facility-loader-kernel.c
?_ltt_facilities_ltt-facility-loader-kernel.h
?_ltt_facilities_ltt-facility-loader-locking.c
?_ltt_facilities_ltt-facility-loader-locking.h
?_ltt_facilities_ltt-facility-loader-memory.c
?_ltt_facilities_ltt-facility-loader-memory.h
?_ltt_facilities_ltt-facility-loader-network.c
?_ltt_facilities_ltt-facility-loader-network.h
?_ltt_facilities_ltt-facility-loader-network_ip_interface.c
?_ltt_facilities_ltt-facility-loader-network_ip_interface.h
?_ltt_facilities_ltt-facility-loader-process.c
?_ltt_facilities_ltt-facility-loader-process.h
?_ltt_facilities_ltt-facility-loader-socket.c
?_ltt_facilities_ltt-facility-loader-socket.h
?_ltt_facilities_ltt-facility-loader-stack_arch_i386.c
?_ltt_facilities_ltt-facility-loader-stack_arch_i386.h
?_ltt_facilities_ltt-facility-loader-stack.c
?_ltt_facilities_ltt-facility-loader-stack.h
?_ltt_facilities_ltt-facility-loader-statedump.c
?_ltt_facilities_ltt-facility-loader-statedump.h
?_ltt_facilities_ltt-facility-loader-timer.c
?_ltt_facilities_ltt-facility-loader-timer.h"


for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-probes.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_probes_ltt-probe-arm.c
?_ltt_probes_ltt-probe-fs.c
?_ltt_probes_ltt-probe-i386.c
?_ltt_probes_ltt-probe-ipc.c
?_ltt_probes_ltt-probe-kernel.c
?_ltt_probes_ltt-probe-list.c
?_ltt_probes_ltt-probe-locking.c
?_ltt_probes_ltt-probe-mips.c
?_ltt_probes_ltt-probe-mm.c
?_ltt_probes_ltt-probe-net.c
?_ltt_probes_ltt-probe-powerpc.c
?_ltt_probes_ltt-probe-ppc.c
?_ltt_probes_ltt-probe-x86_64.c
?_ltt_probes_Makefile"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-statedump.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-statedump.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-build.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_Makefile
?_kernel_Makefile
?_ltt_Kconfig
?_ltt_Makefile
?_arch_alpha_Kconfig
?_arch_cris_Kconfig
?_arch_frv_Kconfig
?_arch_h8300_Kconfig
?_arch_i386_Kconfig
?_arch_ia64_Kconfig
?_arch_m32r_Kconfig
?_arch_m68k_Kconfig
?_arch_m68knommu_Kconfig
?_arch_ppc_Kconfig
?_arch_powerpc_Kconfig
?_arch_parisc_Kconfig
?_arch_arm_Kconfig
?_arch_arm26_Kconfig
?_arch_mips_Kconfig
?_arch_s390_Kconfig
?_arch_sh64_Kconfig
?_arch_sh_Kconfig
?_arch_sparc64_Kconfig
?_arch_sparc_Kconfig
?_arch_um_Kconfig
?_arch_v850_Kconfig
?_arch_xtensa_Kconfig
?_arch_x86_64_Kconfig"

for a in $IN; do wr $a $FILE; done



cd ..

rm $ALL_NAME
tar cvfj ${PRENAME}${NAME}.tar.bz2 ${PRENAME}*${NAME}-*

