## Process this file with automake to produce Makefile.in

LIBS += -lguiEvents

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I/home1/yangxx/poly/include

bin_PROGRAMS = mainwin

mainwin_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	gtkcustom.c gtkcustom.h

mainwin_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

