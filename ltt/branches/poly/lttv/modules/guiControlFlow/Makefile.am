#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on September 27, 2003
#

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -L${top_srcdir}/lttv/modules/gui/API -lmainWinApi

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguiControlFlow.la
libguiControlFlow_la_LDFLAGS = -module
libguiControlFlow_la_SOURCES = 	module.c Event_Hooks.c CFV.c Process_List.c\
				Drawing.c

libguiControlFlow_la_HEADERS = 	Event_Hooks.h CFV.h Process_List.h\
				Drawing.h
