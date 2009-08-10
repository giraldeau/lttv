AM_CFLAGS = $(MODULE_CFLAGS)
AM_LDFLAGS = $(MODULE_LDFLAGS)

LIBS += $(THREAD_LIBS)
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L${top_builddir}/ltt -llttvtraceread
libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la libtextFilter.la libprecomputeState.la libdepanalysis.la

libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_SOURCES = batchAnalysis.c
libtextFilter_la_SOURCES = textFilter.c
libprecomputeState_la_SOURCES = precomputeState.c
libdepanalysis_la_SOURCES = depanalysis.c sstack.c

noinst_HEADERS = \
	batchanalysis.h
