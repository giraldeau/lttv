#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_srcdir}/lttv/modules/gui/mainlib -lmainWinApi

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguievents.la
libguievents_la_LDFLAGS = -module
libguievents_la_SOURCES = events.c
noinst_HEADERS = gtkrbtree.h gtktreeprivate.h

