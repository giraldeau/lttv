#!/bin/sh
# LTTng patch creation 
# Creates a -all patch, and split it.
# Mathieu Desnoyers, october 2005
#$1 is the version

function wr () {
  
  cat $1 >> $2

}


NAME=patch-2.6.17-lttng-core-$1
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



FILE=../$NAME-facilities-headers.diff

IN="?_include_linux_ltt_ltt-facility-core.h
?_include_linux_ltt_ltt-facility-id-core.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-facilities-loader.diff

IN="?_ltt_ltt-facility-loader-core.c
?_ltt_ltt-facility-loader-core.h"

for a in $IN; do wr $a $FILE; done

FILE=../$NAME-facilities.diff

IN="?_include_linux_ltt-facilities.h
?_kernel_ltt-facilities.c"

for a in $IN; do wr $a $FILE; done




FILE=../$NAME-core-timestamp.diff

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
?_include_asm-x86_64_ltt.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-core-header.diff

IN="?_include_linux_ltt-core.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-core.diff

IN="?_MAINTAINERS
?_ltt_ltt-core.c
?_init_main.c
?_kernel_ltt-base.c
?_kernel_ltt-heartbeat.c"

for a in $IN; do wr $a $FILE; done



FILE=../$NAME-transport.diff

IN="?_ltt_ltt-relay.c
?_Documentation_ioctl-number.txt"

for a in $IN; do wr $a $FILE; done



FILE=../$NAME-netlink-control.diff

IN="?_ltt_ltt-control.c
?_ltt_ltt-control.h
?_include_linux_netlink.h"

for a in $IN; do wr $a $FILE; done


FILE=../$NAME-userspace-tracing.diff

IN="?_include_linux_sched.h
?_kernel_sys_ni.c
?_kernel_ltt-syscall.c
?_kernel_exit.c
?_kernel_fork.c
?_include_asm-i386_unistd.h
?_include_asm-powerpc_unistd.h
?_include_asm-x86_64_ia32_unistd.h
?_include_asm-x86_64_unistd.h"

for a in $IN; do wr $a $FILE; done




FILE=../$NAME-build.diff

IN="?_Makefile
?_kernel_Makefile
?_ltt_Kconfig
?_ltt_Makefile
?_arch_i386_Kconfig
?_arch_ppc_Kconfig
?_arch_powerpc_Kconfig
?_arch_arm_Kconfig
?_arch_mips_Kconfig
?_arch_x86_64_Kconfig"

for a in $IN; do wr $a $FILE; done





cd ..

rm $ALL_NAME
tar cvfj $NAME.tar.bz2 $NAME-*

