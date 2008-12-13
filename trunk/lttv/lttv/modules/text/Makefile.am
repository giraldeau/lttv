AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(THREAD_LIBS)
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L${top_builddir}/ltt -llttvtraceread

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la libtextFilter.la libprecomputeState.la
libtextDump_la_LDFLAGS = -module -avoid-version
libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_LDFLAGS = -module -avoid-version
libbatchAnalysis_la_SOURCES = batchAnalysis.c
libtextFilter_la_LDFLAGS = -module -avoid-version
libtextFilter_la_SOURCES = textFilter.c
libprecomputeState_la_LDFLAGS = -module -avoid-version
libprecomputeState_la_SOURCES = precomputeState.c

noinst_HEADERS = \
	batchanalysis.h
