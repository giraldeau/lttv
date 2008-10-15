# SPIN - Verification Software - Version 5.1 - November 2007
#
# Copyright  (c) 1989-2006 Lucent Technologies, Bell Labs
# Extensions (c) 2006-2007 NASA/JPL California Institute of Technology
# All Rights Reserved. For educational purposes only.
# No guarantee whatsoever is expressed or implied by
# the distribution of this code.
#
# Written by:    Gerard J. Holzmann
# Documentation: http://spinroot.com/
# Bug-reports:   bugs@spinroot.com

CC=cc -DNXT 	# -DNXT enables the X operator in LTL
# CC=cc -m32 -DNXT 	# for 32bit compilation on a 64bit system
CFLAGS=-ansi -D_POSIX_SOURCE	# on some systems add: -I/usr/include

# for a more picky compilation:
# CFLAGS=-std=c99 -Wstrict-prototypes -pedantic -fno-strength-reduce -fno-builtin -W -Wshadow -Wpointer-arith -Wcast-qual -Winline -Wall -g

#	on PC:		add -DPC to CFLAGS above
#	on Solaris:	add -DSOLARIS
#	on MAC:		add -DMAC
#	on HP-UX:	add -Aa
#			and add $(CFLAGS) to the spin.o line: $(CC) $(CFLAGS) -c y.tab.c
#	on __FreeBSD__: omit -D_POSIX_SOURCE
#	also recognized: __FreeBSD__ and __OpenBSD__ and __NetBSD__

YACC=yacc		# on Solaris: /usr/ccs/bin/yacc
YFLAGS=-v -d 		# creates y.output and y.tab.h

SPIN_OS= spin.o spinlex.o sym.o vars.o main.o ps_msc.o \
	mesg.o flow.o sched.o run.o pangen1.o pangen2.o \
	pangen3.o pangen4.o pangen5.o guided.o dstep.o \
	structs.o pc_zpp.o pangen6.o reprosrc.o

TL_OS=	tl_parse.o tl_lex.o tl_main.o tl_trans.o tl_buchi.o \
	tl_mem.o tl_rewrt.o tl_cache.o

spin:	$(SPIN_OS) $(TL_OS)
	$(CC) $(CFLAGS) -o spin $(SPIN_OS) $(TL_OS)

spin.o:	spin.y
	$(YACC) $(YFLAGS) spin.y
	$(CC) -c y?tab.c
	rm -f y?tab.c
	mv y?tab.o spin.o

$(SPIN_OS): spin.h

$(TL_OS): tl.h

main.o pangen2.o ps_msc.o: version.h
pangen1.o: pangen1.h pangen3.h pangen6.h
pangen2.o: pangen2.h pangen4.h pangen5.h

# http://spinroot.com/uno/
# using uno version 2.13 -- Oct 2007

uno:	spin.o
	uno_local -picky dstep.c flow.c guided.c main.c mesg.c pangen3.c pangen4.c pangen5.c pangen6.c pc_zpp.c ps_msc.c reprosrc.c run.c sched.c spinlex.c structs.c sym.c tl_buchi.c tl_cache.c tl_lex.c tl_main.c tl_mem.c tl_parse.c tl_rewrt.c tl_trans.c vars.c
	uno_local -picky pangen1.c	# large includes, handle separately for now
	uno_local -picky pangen2.c 
	rm -f spin.o y?tab.? *.uno y.output y.debug

clean:
	rm -f spin *.o y?tab.[ch] y.output y.debug
	rm -f pan.[chmotb] a.out core *stackdump

coverity:
	cov-build --dir covty make
	cov-translate --dir covty gcc -c *.c
	cov-analyze --dir covty
	cov-format-errors --dir covty -x -X
	echo ./covty/output/errors/index.html
