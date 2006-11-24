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

#for hotplug
FILE=../${PRENAME}${COUNT}${NAME}-relay.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_linux_relay.h
?_kernel_relay.c
?_block_blktrace.c"

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

FILE=../${PRENAME}${COUNT}${NAME}-atomic_up.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_asm-i386_atomic-up.h
?_include_asm-i386_system.h
?_include_asm-x86_64_atomic-up.h
?_include_asm-x86_64_system.h
?_include_asm-powerpc_atomic-up.h
?_include_asm-powerpc_system.h
?_include_asm-arm_atomic-up.h
?_include_asm-mips_atomic-up.h
?_include_asm-generic_atomic-up.h"

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
?_include_asm-arm_unistd.h
?_include_asm-i386_unistd.h
?_include_asm-mips_unistd.h
?_include_asm-powerpc_unistd.h
?_include_asm-powerpc_systbl.h
?_include_asm-x86_64_unistd.h"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-build.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_Makefile
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

