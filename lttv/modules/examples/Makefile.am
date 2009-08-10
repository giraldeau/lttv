#
# Makefile for LTT New generation user interface : test plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

AM_CFLAGS = $(PACKAGE_CFLAGS)
AM_LDFLAGS = $(MODULE_LDFLAGS)

LIBS += $(GLIB_LIBS)

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libsampledep.la libsamplemodule.la libsamplemodule2.la
libsampledep_la_SOURCES = sampledep.c
libsamplemodule_la_SOURCES = samplemodule.c
libsamplemodule2_la_SOURCES = samplemodule2.c
