SUBDIRS = modules

AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS)

bin_PROGRAMS = tracevisualizer

#AM_CPPFLAGS = \
#	-I$(top_srcdir)/include
#INCLUDES=$(top_srcdir)/include

tracevisualizer_SOURCES = main.c module.c option.c hook.c attribute.c \
		 attribute.h lttv.h option.h trace.h
