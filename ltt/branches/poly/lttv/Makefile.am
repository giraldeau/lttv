AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread
#LIBS += $(GLIB_LIBS) -lgobject-2.0 -lltt

bin_PROGRAMS = lttv

#AM_CPPFLAGS = \
#	-I$(top_srcdir)/include
#INCLUDES=$(top_srcdir)/include

lttv_SOURCES = main.c module.c option.c hook.c attribute.c \
		iattribute.c processTrace.c state.c stats.c traceset.c

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c


# WARNING : must be done at the end, so modules can dynamically link themselves
# to libraries compiled here but not installed in the system.
SUBDIRS = modules

