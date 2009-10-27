AM_CFLAGS = $(MODULE_CFLAGS)
AM_LDFLAGS = $(MODULE_LDFLAGS)

libdir = ${lttvplugindir}

lib_LTLIBRARIES = libtextDump.la libbatchAnalysis.la libtextFilter.la libprecomputeState.la libdepanalysis.la libsync_chain_batch.la

libtextDump_la_SOURCES = textDump.c
libbatchAnalysis_la_SOURCES = batchAnalysis.c
libtextFilter_la_SOURCES = textFilter.c
libprecomputeState_la_SOURCES = precomputeState.c
libdepanalysis_la_SOURCES = depanalysis.c sstack.c
libsync_chain_batch_la_SOURCES = sync_chain_batch.c

noinst_HEADERS = \
	batchanalysis.h \
	sstack.h
