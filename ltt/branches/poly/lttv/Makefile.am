bin_PROGRAMS = textui

# Includes will have to be changed in the .c files if we want to
# remove the top_srcdir from the includes path.
AM_CPPFLAGS = \
	-I$(top_srcdir)/lttv \
	-I$(top_srcdir)

# MD //FIXME add headers dependancy
textui_SOURCES = main.c module.c option.c hook.c attribute.c
textui_LDFLAGS = -lpopt -lgmodule-2.0 -ldl -lglib-2.0
