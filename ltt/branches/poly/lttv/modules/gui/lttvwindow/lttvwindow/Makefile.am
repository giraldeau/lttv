## Process this file with automake to produce Makefile.in

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(M_LIBS) $(GLIB_LIBS)
LIBS += $(GTK_LIBS)

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	$(DEFAULT_INCLUDES)

libdir = ${lttvplugindir}

lib_LTLIBRARIES = liblttvwindow.la


liblttvwindow_la_SOURCES = \
	toolbar.c\
	menu.c\
	gtkdirsel.c\
	lttvwindow.c \
	lttvwindowtraces.c \
	init_module.c \
	support.c \
	interface.c \
	callbacks.c \
	lttvfilter.c

noinst_HEADERS = \
	support.h \
	interface.h \
	callbacks.h\
	mainwindow-private.h

lttvwindowinclude_HEADERS = \
	gtkdirsel.h\
	lttvwindow.h\
	lttvwindowtraces.h\
	lttvfilter.h\
	mainwindow.h\
	menu.h\
	toolbar.h

liblttvwindow_la_LIBADD = -lm @PACKAGE_LIBS@ $(INTLLIBS)

