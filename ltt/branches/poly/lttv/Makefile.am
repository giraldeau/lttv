SUBDIRS = modules

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread
#LIBS += $(GLIB_LIBS) -lgobject-2.0 -lltt

bin_PROGRAMS = lttv

#AM_CPPFLAGS = \
#	-I$(top_srcdir)/include
#INCLUDES=$(top_srcdir)/include

lttv_SOURCES = main.c module.c option.c hook.c attribute.c \
		iattribute.c processTrace.c state.c traceset.c

lttv_DEPENDENCIES = $(top_includedir)/lttv/lttv.h \
	$(top_includedir)/lttv/module.h \
	$(top_includedir)/lttv/option.h \
	$(top_includedir)/lttv/hook.h \
	$(top_includedir)/lttv/attribute.h \
	$(top_includedir)/lttv/iattribute.h \
	$(top_includedir)/lttv/processTrace.h \
	$(top_includedir)/lttv/state.h \
	$(top_includedir)/lttv/traceset.h


libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c
