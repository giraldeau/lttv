SUBDIRS = modules

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

bin_PROGRAMS = lttv

#AM_CPPFLAGS = \
#	-I$(top_srcdir)/include
#INCLUDES=$(top_srcdir)/include

tracevisualizer_SOURCES = main.c module.c option.c hook.c attribute.c \
		iattribute.c processTrace.c state.c traceset.c
#		lttv.h module.h option.h hook.h attribute.h \
#                iattribute.h processTrace.h state.h traceset.h


libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c
