AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(POPT_LIBS) $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread

bin_PROGRAMS = lttv

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

lttv_SOURCES = batchtest.c main.c module.c option.c \
               hook.c attribute.c \
               iattribute.c state.c stats.c \
	       tracecontext.c traceset.c filter.c print.c

if LTTVSTATIC
  lttv_LDFLAGS = -profile -static
endif
