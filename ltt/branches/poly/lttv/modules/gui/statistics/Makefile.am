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

lib_LTLIBRARIES = libguistatistics.la
libguiStatistic_la_LDFLAGS = -module
libguiStatistic_la_SOURCES = statistics.c
