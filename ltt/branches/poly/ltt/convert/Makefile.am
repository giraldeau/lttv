AM_CFLAGS = $(GLIB_CFLAGS) 
LIBS += $(GLIB_LIBS) $(M_LIBS)

bin_PROGRAMS = convert

convert_SOURCES = convert.c

noinst_HEADERS = LTTTypes.h LinuxEvents.h

EXTRA_DIST = core.xml sysInfo
convertdir = $(pkgdatadir)/convert
convert_DATA = core.xml sysInfo
