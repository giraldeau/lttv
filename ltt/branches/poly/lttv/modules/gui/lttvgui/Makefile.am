#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

libdir = ${lttvlibdir}

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS)

lib_LTLIBRARIES = liblttvgui.la
liblttvgui_la_SOURCES = toolbar.c menu.c gtktraceset.c
#liblttvgui_la_LDFLAGS = -L${top_srcdir}/lttv/modules/gui/lttvgui -lcustomBox

lttvguiinclude_HEADERS = \
	common.h\
	gtkdirsel.h\
	gtkmultivpaned.h\
	gtktraceset.h\
	lttvfilter.h\
	mainwindow.h\
	menu.h\
	toolbar.h

EXTRA_DIST =
