## Process this file with automake to produce Makefile.in

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
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
	lttvwindow.c \
	lttvwindowtraces.c \
	init_module.c \
	support.c \
	interface.c \
	callbacks.c

noinst_HEADERS = \
	support.h \
	interface.h \
	callbacks.h\
	mainwindow-private.h

lttvwindowinclude_HEADERS = \
	lttvwindow.h\
	lttvwindowtraces.h\
	mainwindow.h\
	menu.h\
	toolbar.h

liblttvwindow_la_LIBADD = @PACKAGE_LIBS@ $(INTLLIBS)

