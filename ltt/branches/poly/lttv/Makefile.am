AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) -lgobject-2.0 -L$(top_srcdir)/ltt -ltraceread

bin_PROGRAMS = lttv

lttv_SOURCES = main/main.c mainLib/module.c mainLib/option.c \
               mainLib/hook.c mainLib/attribute.c \
               mainLib/iattribute.c mainLib/processTrace.c \
							 mainLib/state.c mainLib/stats.c mainLib/traceset.c

# WARNING : must be done at the end, so modules can dynamically link themselves
# to libraries compiled here but not installed in the system.
SUBDIRS = modules

