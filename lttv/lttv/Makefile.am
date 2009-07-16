AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(THREAD_LIBS)
LIBS += $(POPT_LIBS) $(GLIB_LIBS) -lgobject-2.0 -L${top_builddir}/ltt\
          -llttvtraceread

bin_PROGRAMS = lttv.real

bin_SCRIPTS = lttv lttv-gui
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = lttv.sh lttv-gui.sh

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
	@PACKAGE_CFLAGS@ \
	$(DEFAULT_INCLUDES)

libdir = ${lttvplugindir}

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

#noinst_HEADERS = \
#	filter.h

lttv_real_SOURCES = batchtest.c main.c module.c option.c \
               hook.c attribute.c \
               iattribute.c state.c stats.c \
	       tracecontext.c traceset.c filter.c print.c

#man_MANS = lttv.1
#EXTRA_DIST = lttv.1

#install-data-hook:
#	cd $(DESTDIR)$(mandir)/man1 && \
#		$(LN_S) -f lttv.1 lttv-gui.1 \
#		$(LN_S) -f lttv.1 lttv.real.1

lttv_real_LDFLAGS = -export-dynamic

if LTTVSTATIC
  lttv_real_LDFLAGS += -profile -static
endif

