#!/bin/sh
# LTTng patch creation 
# Creates a -all patch, and split it.
# Mathieu Desnoyers, october 2005
#$1 is the version

function wr () {
  
  cat $1 >> $2

}


PRENAME=patch
NAME=-2.6.21-rc6-mm1-lttng-$1
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

#hotfix 2.6.20
FILE=../${PRENAME}${COUNT}${NAME}-hotfix.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-powerpc_prom.h
?_include_asm-sparc64_tlb.h
?_kernel_time_clocksource.c
?_include_asm-ppc_ocp.h
?_arch_powerpc_kernel_setup_32.c
?_arch_ppc_kernel_setup.c
?_arch_ppc_kernel_ppc_ksyms.c
?_arch_sparc64_kernel_process.c
?_arch_sparc_kernel_process.c
?_arch_sparc_kernel_traps.c
?_arch_avr32_kernel_ptrace.c"
#also in instrumentation
#?_arch_x86_64_kernel_process.c
#?_arch_powerpc_kernel_process.c

for a in $IN; do wr $a $FILE; done


#for hotplug
FILE=../${PRENAME}${COUNT}${NAME}-relay.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_relay.h
?_kernel_relay.c
?_Documentation_filesystems_relay.txt
?_block_blktrace.c"

for a in $IN; do wr $a $FILE; done


#Markers

FILE=../${PRENAME}${COUNT}${NAME}-markers-kconfig.part1.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_kernel_Kconfig.marker"

for a in $IN; do wr $a $FILE; done


#marker linker scripts
FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-generic.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-generic_vmlinux.lds.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-alpha.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_alpha_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-arm.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_arm_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-arm26.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_arm26_kernel_vmlinux-arm26-xip.lds.in
?_arch_arm26_kernel_vmlinux-arm26.lds.in"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-avr32.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_avr32_kernel_vmlinux.lds.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-cris.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_cris_arch-v10_vmlinux.lds.S
?_arch_cris_arch-v32_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-frv.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_frv_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-h8300.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_h8300_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_i386_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-ia64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_ia64_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-m32r.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_m32r_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-m68k.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_m68k_kernel_vmlinux-std.lds
?_arch_m68k_kernel_vmlinux-sun3.lds"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-m68knommu.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_m68knommu_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-mips.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_mips_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-parisc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_parisc_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_powerpc_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-ppc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_ppc_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-s390.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_s390_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-sh.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sh_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-sh64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sh64_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-sparc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sparc_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-sparc64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sparc64_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-um.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_um_kernel_dyn.lds.S
?_arch_um_kernel_uml.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-v850.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_v850_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-x86_64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_x86_64_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-linker-scripts-xtensa.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_xtensa_kernel_vmlinux.lds.S"

for a in $IN; do wr $a $FILE; done






#markers implementation
FILE=../${PRENAME}${COUNT}${NAME}-markers-generic.part1.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_module.h
?_include_linux_marker.h
?_include_linux_kernel.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-markers-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-i386_marker.h
?_arch_i386_kernel_marker.c
?_arch_i386_kernel_Makefile"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-markers-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-powerpc_marker.h
?_arch_powerpc_kernel_marker.c
?_arch_powerpc_kernel_Makefile"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-markers-non-opt-arch.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-alpha_marker.h
?_include_asm-arm_marker.h
?_include_asm-arm26_marker.h
?_include_asm-cris_marker.h
?_include_asm-frv_marker.h
?_include_asm-generic_marker.h
?_include_asm-h8300_marker.h
?_include_asm-ia64_marker.h
?_include_asm-m32r_marker.h
?_include_asm-m68k_marker.h
?_include_asm-m68knommu_marker.h
?_include_asm-mips_marker.h
?_include_asm-parisc_marker.h
?_include_asm-ppc_marker.h
?_include_asm-s390_marker.h
?_include_asm-sh_marker.h
?_include_asm-sh64_marker.h
?_include_asm-sparc_marker.h
?_include_asm-sparc64_marker.h
?_include_asm-um_marker.h
?_include_asm-v850_marker.h
?_include_asm-x86_64_marker.h
?_include_asm-xtensa_marker.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-markers-doc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_Documentation_marker.txt"

for a in $IN; do wr $a $FILE; done


#atomic

FILE=../${PRENAME}${COUNT}${NAME}-atomic-alpha.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-alpha_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-avr32.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-avr32_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-frv.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-frv_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-atomic-generic-atomic_long.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-generic_atomic.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-atomic-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-i386_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-ia64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-ia64_atomic.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-atomic-mips.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-mips_atomic.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-atomic-parisc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-parisc_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-powerpc_atomic.h
?_include_asm-powerpc_bitops.h
?_include_asm-powerpc_system.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-ppc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-ppc_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-atomic-sparc64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-sparc64_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-s390.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-s390_atomic.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-atomic-x86_64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-x86_64_atomic.h"

for a in $IN; do wr $a $FILE; done



#local

FILE=../${PRENAME}${COUNT}${NAME}-local-generic.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-generic_local.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-local-documentation.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_Documentation_local_ops.txt"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-local-alpha.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-alpha_local.h
?_include_asm-alpha_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-i386_local.h
?_include_asm-i386_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-ia64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-ia64_local.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-mips.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-mips_local.h
?_include_asm-mips_system.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-parisc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-parisc_local.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-powerpc_local.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-s390.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-s390_local.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-sparc64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-sparc64_local.h"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-local-x86_64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-x86_64_local.h
?_include_asm-x86_64_system.h"


#facilities

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
?_include_asm-arm_ltt.h
?_include_asm-arm26_ltt.h
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
?_include_asm-mips_timex.h
?_arch_mips_kernel_time.c
?_include_asm-parisc_ltt.h
?_include_asm-powerpc_ltt.h
?_include_asm-ppc_ltt.h
?_include_asm-s390_ltt.h
?_include_asm-sh_ltt.h
?_include_asm-sh64_ltt.h
?_include_asm-sparc_ltt.h
?_include_asm-sparc64_ltt.h
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

IN="?_include_linux_ltt-tracer.h"

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
?_include_asm-arm_unistd.h
?_include_asm-i386_unistd.h
?_include_asm-mips_unistd.h
?_include_asm-powerpc_systbl.h
?_include_asm-powerpc_unistd.h
?_include_asm-x86_64_unistd.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-serialize.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-serialize.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-test-tsc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-test-tsc.c"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-arm.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_arm_kernel_calls.S
?_arch_arm_kernel_entry-common.S
?_arch_arm_kernel_process.c
?_arch_arm_kernel_ptrace.c
?_arch_arm_kernel_sys_arm.c
?_arch_arm_kernel_time.c
?_arch_arm_kernel_traps.c
?_include_asm-arm_thread_info.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-i386.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_i386_kernel_process.c
?_arch_i386_kernel_ptrace.c
?_arch_i386_kernel_sys_i386.c
?_arch_i386_kernel_syscall_table.S
?_arch_i386_kernel_time.c
?_arch_i386_kernel_traps.c
?_arch_i386_mm_fault.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-mips.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_mips_kernel_process.c
?_arch_mips_kernel_ptrace.c
?_arch_mips_kernel_scall32-o32.S
?_arch_mips_kernel_scall64-64.S
?_arch_mips_kernel_scall64-n32.S
?_arch_mips_kernel_scall64-o32.S
?_arch_mips_kernel_syscall.c
?_arch_mips_kernel_traps.c
?_arch_mips_kernel_unaligned.c
?_include_asm-mips_mipsregs.h
?_arch_mips_mm_fault.c"

for a in $IN; do wr $a $FILE; done



FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-powerpc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_powerpc_kernel_irq.c
?_arch_powerpc_kernel_misc_32.S
?_arch_powerpc_kernel_misc_64.S
?_arch_powerpc_kernel_ppc_ksyms.c
?_arch_powerpc_kernel_process.c
?_arch_powerpc_kernel_prom.c
?_arch_powerpc_kernel_ptrace.c
?_arch_powerpc_kernel_syscalls.c
?_arch_powerpc_kernel_time.c
?_arch_powerpc_kernel_traps.c
?_arch_powerpc_mm_fault.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-ppc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_ppc_kernel_misc.S
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
?_arch_x86_64_kernel_init_task.c
?_arch_x86_64_kernel_process.c
?_arch_x86_64_kernel_ptrace.c
?_arch_x86_64_kernel_time.c
?_arch_x86_64_kernel_traps.c
?_arch_x86_64_mm_fault.c"
#?_arch_x86_64_kernel_init_task.c is for stack dump as module

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-m68k.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-m68k_thread_info.h"

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-m68knommu.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-m68knommu_thread_info.h
?_arch_m68knommu_platform_68328_entry.S"

for a in $IN; do wr $a $FILE; done


#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-sparc.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-sparc_thread_info.h
?_arch_sparc_kernel_entry.S"

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-s390.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_s390_kernel_traps.c
?_arch_s390_kernel_sys_s390.c
?_arch_s390_mm_fault.c"

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-sh.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sh_kernel_entry-common.S
?_arch_sh_kernel_irq.c
?_arch_sh_kernel_process.c
?_arch_sh_kernel_sys_sh.c
?_arch_sh_kernel_traps.c
?_arch_sh_mm_fault.c
?_include_asm-sh_thread_info.h"

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-sh64.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_arch_sh64_kernel_entry.S
?_include_asm-sh64_thread_info.h"

for a in $IN; do wr $a $FILE; done

#limited
FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-alpha.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-alpha_thread_info.h"

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
?_kernel_lockdep.c
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
?_net_socket.c
?_kernel_extable.c"
#extable is for stack dump as module : __kernel_text_address must be exported

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-instrumentation-markers.tosplit.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_kernel_module.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-probes.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_probes_Makefile
?_ltt_probes_ltt-probe-compact.c
?_ltt_probes_ltt-probe-core.c
?_ltt_probes_ltt-probe-fs.c
?_ltt_probes_ltt-probe-kernel.c
?_ltt_probes_ltt-probe-kernel_arch_arm.c
?_ltt_probes_ltt-probe-kernel_arch_i386.c
?_ltt_probes_ltt-probe-kernel_arch_mips.c
?_ltt_probes_ltt-probe-kernel_arch_powerpc.c
?_ltt_probes_ltt-probe-kernel_arch_ppc.c
?_ltt_probes_ltt-probe-kernel_arch_x86_64.c
?_ltt_probes_ltt-probe-list.c
?_ltt_probes_ltt-probe-locking.c
?_ltt_probes_ltt-probe-mm.c
?_ltt_probes_ltt-probe-net.c
?_ltt_probes_ltt-probe-stack_arch_i386.c
?_ltt_probes_ltt-probe-stack_arch_x86_64.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-statedump.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-statedump.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-build.tosplit.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_Makefile
?_ltt_Kconfig
?_ltt_Makefile
?_arch_alpha_Kconfig
?_arch_arm26_Kconfig
?_arch_arm_Kconfig
?_arch_avr32_Kconfig.debug
?_arch_cris_Kconfig
?_arch_frv_Kconfig
?_arch_h8300_Kconfig
?_arch_i386_Kconfig
?_arch_ia64_Kconfig
?_arch_m32r_Kconfig
?_arch_m68k_Kconfig
?_arch_m68knommu_Kconfig
?_arch_mips_Kconfig
?_arch_parisc_Kconfig
?_arch_powerpc_Kconfig
?_arch_ppc_Kconfig
?_arch_s390_Kconfig
?_arch_sh_Kconfig
?_arch_sh64_Kconfig
?_arch_sparc_Kconfig
?_arch_sparc64_Kconfig
?_arch_um_Kconfig
?_arch_v850_Kconfig
?_arch_x86_64_Kconfig
?_arch_xtensa_Kconfig"

for a in $IN; do wr $a $FILE; done



cd ..

rm $ALL_NAME
tar cvfj ${PRENAME}${NAME}.tar.bz2 ${PRENAME}*${NAME}-*
