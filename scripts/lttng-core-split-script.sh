# LTTng patch creation core
PRENAME=patch
NAME=-2.6.18-lttng-core-$1
ALL_NAME=${PRENAME}${NAME}-all.diff
VALUE=1
printf -v COUNT "%02d" ${VALUE}
FILE=../${PRENAME}${COUNT}${NAME}-debugfs.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
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
?_include_asm-xtensa_marker.h
?_kernel_module.c"
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
FILE=../${PRENAME}${COUNT}${NAME}-facilities.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
?_ltt_ltt-facilities.c"
FILE=../${PRENAME}${COUNT}${NAME}-facility-core-headers.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_ltt_ltt-facility-core.h
?_include_ltt_ltt-facility-id-core.h
?_include_ltt_ltt-facility-select-core.h"

for a in $IN; do wr $a $FILE; done
FILE=../${PRENAME}${COUNT}${NAME}-facility-loader-core.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_facilities_ltt-facility-loader-core.c
?_ltt_facilities_ltt-facility-loader-core.h
?_ltt_facilities_Makefile"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-timestamp.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
?_include_asm-x86_64_ltt.h
?_include_asm-xtensa_ltt.h"
FILE=../${PRENAME}${COUNT}${NAME}-core-header.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
FILE=../${PRENAME}${COUNT}${NAME}-core.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
?_ltt_ltt-heartbeat.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-tracer-header.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_include_ltt_ltt-tracer.h"
FILE=../${PRENAME}${COUNT}${NAME}-tracer.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}

IN="?_ltt_ltt-tracer.c"

for a in $IN; do wr $a $FILE; done


FILE=../${PRENAME}${COUNT}${NAME}-transport.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
FILE=../${PRENAME}${COUNT}${NAME}-netlink-control.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
FILE=../${PRENAME}${COUNT}${NAME}-userspace-tracing.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
?_ltt_ltt-syscall.c
?_include_asm-arm_unistd.h
?_include_asm-mips_unistd.h
?_include_asm-powerpc_systbl.h
FILE=../${PRENAME}${COUNT}${NAME}-build.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
?_arch_alpha_Kconfig
?_arch_cris_Kconfig
?_arch_frv_Kconfig
?_arch_h8300_Kconfig
?_arch_ia64_Kconfig
?_arch_m32r_Kconfig
?_arch_m68k_Kconfig
?_arch_m68knommu_Kconfig
?_arch_parisc_Kconfig
?_arch_arm26_Kconfig
?_arch_s390_Kconfig
?_arch_sh64_Kconfig
?_arch_sh_Kconfig
?_arch_sparc64_Kconfig
?_arch_sparc_Kconfig
?_arch_um_Kconfig
?_arch_v850_Kconfig
?_arch_xtensa_Kconfig
tar cvfj ${PRENAME}${NAME}.tar.bz2 ${PRENAME}*${NAME}-*