## Process this file with automake to produce Makefile.in

AM_CFLAGS = -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DPACKAGE_BIN_DIR=\""$(bindir)"\"

bin_PROGRAMS = lttctl

lttctl_SOURCES = \
	lttctl.c
lttctl_DEPENDENCIES = ../libltt/libltt.la
lttctl_LDADD = $(lttctl_DEPENDENCIES)

