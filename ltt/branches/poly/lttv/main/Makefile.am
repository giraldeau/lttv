AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread

bin_PROGRAMS = lttv

INCLUDES = \
	-DPACKAGE_PLUGIN_DIR=\""$(lttvplugindir)"\" \
	@PACKAGE_CFLAGS@ \
	$(DEFAULT_INCLUDES)

libdir = ${lttvplugindir}

lttv_SOURCES = main.c module.c option.c \
               hook.c attribute.c \
               iattribute.c processTrace.c \
	       state.c stats.c traceset.c

