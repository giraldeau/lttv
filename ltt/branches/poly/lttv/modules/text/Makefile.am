

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(M_LIBS) $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libtextDump_la_LIBADD = -lm
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c
libbatchAnalysis_la_LIBADD = -lm

noinst_HEADERS = \
	batchanalysis.h
