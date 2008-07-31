#
# Makefile for LTT New generation user interface : test plugins.
#
# Created by Mathieu Desnoyers on May 6, 2003
#

libdir = ${lttvplugindir}
AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

lib_LTLIBRARIES = libsampledep.la libsamplemodule.la libsamplemodule2.la
libsampledep_la_LDFLAGS = -module -avoid-version
libsampledep_la_SOURCES = sampledep.c
libsamplemodule_la_LDFLAGS = -module -avoid-version
libsamplemodule_la_SOURCES = samplemodule.c
libsamplemodule2_la_LDFLAGS = -module -avoid-version
libsamplemodule2_la_SOURCES = samplemodule2.c
