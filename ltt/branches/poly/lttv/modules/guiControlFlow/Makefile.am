#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

# the gui subdir comes first : dependency
SUBDIRS = gui

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -Lgui/API -lmainWinApi -lcustomBox

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguiEvents.la # libguiControlFlow.la
libguiEvents_la_LDFLAGS = -module
libguiEvents_la_SOURCES = guiEvents.c
libguiControlFlow_la_LDFLAGS = -module
libguiControlFlow_la_SOURCES = guiControlFlow.c
