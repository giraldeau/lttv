

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L${top_srcdir}/ltt -llttvtraceread

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la libtextFilter.la
libtextDump_la_LDFLAGS = -module
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module
libbatchAnalysis_la_SOURCES = batchAnalysis.c
libtextFilter_la_LDFLAGS = -module
libtextFilter_la_SOURCES = textFilter.c
libprecomputeState_la_LDFLAGS = -module
libprecomputeState_la_SOURCES = precomputeState.c

noinst_HEADERS = \
	batchanalysis.h
