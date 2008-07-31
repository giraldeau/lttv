## Process this file with automake to produce Makefile.in

AM_CFLAGS = -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DPACKAGE_BIN_DIR=\""$(bindir)"\"

bin_PROGRAMS = lttctl
bin_SCRIPTS = ltt-armall ltt-disarmall ltt-armalluser ltt-disarmalluser
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = ltt-armall.sh ltt-disarmall.sh ltt-armalluser.sh ltt-disarmalluser.sh

ltt-armall: ltt-armall.sh
	rm -f ltt-armall
	echo "#!"$(BASH) > ltt-armall
	cat $(srcdir)/ltt-armall.sh >> ltt-armall
	chmod ugo+x ltt-armall

ltt-disarmall: ltt-disarmall.sh
	rm -f ltt-disarmall
	echo "#!"$(BASH) > ltt-disarmall
	cat $(srcdir)/ltt-disarmall.sh >> ltt-disarmall
	chmod ugo+x ltt-disarmall

ltt-armalluser: ltt-armalluser.sh
	rm -f ltt-armalluser
	echo "#!"$(BASH) > ltt-armalluser
	cat $(srcdir)/ltt-armalluser.sh >> ltt-armalluser
	chmod ugo+x ltt-armalluser

ltt-disarmalluser: ltt-disarmalluser.sh
	rm -f ltt-disarmalluser
	echo "#!"$(BASH) > ltt-disarmalluser
	cat $(srcdir)/ltt-disarmalluser.sh >> ltt-disarmalluser
	chmod ugo+x ltt-disarmalluser

lttctl_SOURCES = \
	lttctl.c
lttctl_DEPENDENCIES = ../liblttctl/liblttctl.la
lttctl_LDADD = $(lttctl_DEPENDENCIES)

