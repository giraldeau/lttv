SUBDIRS= sync

AM_CFLAGS= $(PACKAGE_CFLAGS)
LDADD = $(POPT_LIBS) $(M_LIBS) $(GLPK_LIBS) ${top_builddir}/ltt/liblttvtraceread.la

bin_PROGRAMS = lttv.real

if BUILD_LTTV_GUI
bin_SCRIPTS = lttv lttv-gui
else
bin_SCRIPTS = lttv
endif

CLEANFILES = $(bin_SCRIPTS)

if BUILD_LTTV_GUI
EXTRA_DIST = lttv.sh lttv-gui.sh
else
EXTRA_DIST = lttv.sh
endif

lttv: lttv.sh
	rm -f lttv
	echo "#!"$(BASH) > lttv
	cat $(srcdir)/lttv.sh >> lttv
	chmod ugo+x lttv

lttv-gui: lttv-gui.sh
	rm -f lttv-gui
	echo "#!"$(BASH) > lttv-gui
	cat $(srcdir)/lttv-gui.sh >> lttv-gui
	chmod ugo+x lttv-gui


INCLUDES = \
	-DPACKAGE_PLUGIN_DIR=\""$(lttvplugindir)"\" \
	$(DEFAULT_INCLUDES)

libdir = ${lttvplugindir}

#noinst_HEADERS = \
#	filter.h

lttv_real_SOURCES = \
	batchtest.c\
	main.c\
	module.c\
	option.c\
	hook.c\
	attribute.c\
	iattribute.c\
	state.c\
	stats.c\
	tracecontext.c\
	traceset.c\
	filter.c\
	print.c\
	sync/sync_chain.c\
	sync/sync_chain.h\
	sync/sync_chain_lttv.c\
	sync/sync_chain_lttv.h\
	sync/graph_functions.c\
	sync/graph_functions.h\
	sync/data_structures.c\
	sync/data_structures.h\
	sync/event_processing.h\
	sync/event_processing_lttng_common.c\
	sync/event_processing_lttng_common.h\
	sync/event_processing_lttng_standard.c\
	sync/event_processing_lttng_standard.h\
	sync/event_processing_lttng_null.c\
	sync/event_processing_lttng_null.h\
	sync/event_matching.h\
	sync/event_matching_broadcast.c\
	sync/event_matching_broadcast.h\
	sync/event_matching_distributor.c\
	sync/event_matching_distributor.h\
	sync/event_matching_tcp.c\
	sync/event_matching_tcp.h\
	sync/event_analysis.h\
	sync/event_analysis_chull.c\
	sync/event_analysis_chull.h\
	sync/event_analysis_eval.c\
	sync/event_analysis_eval.h\
	sync/event_analysis_linreg.c\
	sync/event_analysis_linreg.h\
	sync/lookup3.h

lttvinclude_HEADERS = \
	attribute.h\
	hook.h\
	iattribute.h\
	lttv.h\
	module.h\
	option.h\
	state.h\
	stats.h\
	tracecontext.h\
	traceset.h\
	filter.h\
	print.h

#man_MANS = lttv.1
#EXTRA_DIST = lttv.1

#install-data-hook:
#	cd $(DESTDIR)$(mandir)/man1 && \
#		$(LN_S) -f lttv.1 lttv-gui.1 \
#		$(LN_S) -f lttv.1 lttv.real.1

lttv_real_LDFLAGS = -export-dynamic

if LTTVSTATIC
  lttv_real_LDFLAGS += -static
endif

