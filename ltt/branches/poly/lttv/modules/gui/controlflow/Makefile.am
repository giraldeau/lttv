#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on September 27, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_srcdir}/lttv/modules/gui/mainlib -lmainWinApi

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguicontrolflow.la
libguicontrolflow_la_LDFLAGS = -module
libguicontrolflow_la_SOURCES = 	module.c eventhooks.c cfv.c processlist.c\
				drawing.c drawitem.c

noinst_HEADERS = 	eventhooks.h cfv.h cfv-private.h processlist.h\
				drawing.h drawitem.h
