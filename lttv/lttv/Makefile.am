AM_CFLAGS= $(PACKAGE_CFLAGS)
LDADD = $(POPT_LIBS) $(M_LIBS) ${top_builddir}/ltt/liblttvtraceread.la

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
	sync/data_structures_tcp.c\
	sync/event_processing_lttv_common.c\
	sync/event_processing_lttv_standard.c\
	sync/event_processing_lttv_null.c\
	sync/event_matching_tcp.c\
	sync/event_analysis_linreg.c

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

