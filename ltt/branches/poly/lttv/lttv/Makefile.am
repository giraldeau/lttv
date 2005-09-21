AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(POPT_LIBS) $(GLIB_LIBS) -lgobject-2.0 -L${top_srcdir}/ltt\
          -llttvtraceread

bin_PROGRAMS = lttv.real

bin_SCRIPTS = lttv lttv-gui
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = lttv.sh lttv-gui.sh

lttv: lttv.sh
	rm -f lttv
	echo "#!"$(BASH) > lttv
	cat lttv.sh >> lttv
	chmod ugo+x lttv

lttv-gui: lttv-gui.sh
	rm -f lttv-gui
	echo "#!"$(BASH) > lttv-gui
	cat lttv-gui.sh >> lttv-gui
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

if LTTVSTATIC
  lttv_real_LDFLAGS = -profile -static
endif
