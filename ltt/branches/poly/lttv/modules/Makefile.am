#
# Makefile for LTT New generation user interface : plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

SUBDIRS = gui

AM_CFLAGS = $(GLIB_CFLAGS) 
AM_CFLAGS += $(GTK_CFLAGS)
LIBS += $(GLIB_LIBS)
LIBS += $(GTK_LIBS) -Lgui/API -lmainWinApi

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libguiEvents.la
libguiEvents_la_LDFLAGS = -module
libguiEvents_la_SOURCES = guiEvents.c
