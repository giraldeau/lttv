## Process this file with automake to produce Makefile.in

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L$(top_srcdir)/lttv/modules/gui/API -lmainWinApi

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	$(DEFAULT_INCLUDES)

libdir = ${lttvplugindir}

#bin_PROGRAMS = mainwin
lib_LTLIBRARIES = libmainwin.la

libmainwin_la_LDFLAGS = -module
libmainwin_la_SOURCES = \
	init_module.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	gtkcustom.c gtkcustom.h

libmainwin_la_LIBADD = @PACKAGE_LIBS@ $(INTLLIBS)

