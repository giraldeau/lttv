SUBDIRS = modules

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -lltt

bin_PROGRAMS = lttv

#AM_CPPFLAGS = \
#	-I$(top_srcdir)/include
#INCLUDES=$(top_srcdir)/include

lttv_SOURCES = main.c module.c option.c hook.c attribute.c \
		iattribute.c processTrace.c state.c traceset.c

#lttv_DEPENDENCIES = $(DEFAULT_INCLUDES)/lttv/lttv.h \
#	$(DEFAULT_INCLUDES)/lttv/module.h \
#	$(DEFAULT_INCLUDES)/lttv/option.h \
#	$(DEFAULT_INCLUDES)/lttv/hook.h \
#	$(DEFAULT_INCLUDES)/lttv/attribute.h \
#	$(DEFAULT_INCLUDES)/lttv/iattribute.h \
#	$(DEFAULT_INCLUDES)/lttv/processTrace.h \
#	$(DEFAULT_INCLUDES)/lttv/state.h \
#	$(DEFAULT_INCLUDES)/lttv/traceset.h


libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c
