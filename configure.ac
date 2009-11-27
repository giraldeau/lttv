# This file is part of the Linux Trace Toolkit viewer
# Copyright (C) 2003-2004 Mathieu Desnoyers
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License Version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
# MA 02111-1307, USA.



#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([lttv], [0.12.24-27112009], [ltt-dev@lists.casi.polymtl.ca])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AM_PROG_LIBTOOL

# If requested, only build LTT library part
AC_ARG_WITH(lttv, [  --with-lttv             build whole LTTV system, as opposd to only the trace reading library
			  [[default=yes]]])
if test -z "$with_lttv"; then
        with_lttv=${with_lttv_default-yes}
else
	if test "$with_lttv" = "no"; then
	        # Requesting lib-only turn off the lttv-gui building as well
        	with_lttv_gui="no"
	fi
fi
AM_CONDITIONAL(BUILD_LIB_ONLY, test "$with_lttv" = "no")


# If requested, avoid building LTTV part
AC_ARG_WITH(lttv-gui, [  --with-lttv-gui         build LTTV gui, as opposed to only build textmode LTTV
			  [[default=yes]]])

if test -z "$with_lttv_gui"; then
        with_lttv_gui=${with_lttv_gui_default-yes}
fi
AM_CONDITIONAL(BUILD_LTTV_GUI, test "$with_lttv_gui" = "yes")

AM_PATH_GLIB_2_0(2.4.0, ,AC_MSG_ERROR([glib is required in order to compile LinuxTraceToolkit - download it from ftp://ftp.gtk.org/pub/gtk]) , gmodule)

# GTK is only needed by the GUI
if test "$with_lttv_gui" = "yes" ; then
        AM_PATH_GTK_2_0(2.4.0, ,AC_MSG_ERROR([gtk is required in order to compile GUI - download it from ftp://ftp.gtk.org/pub/gtk]) , gmodule)
fi

AC_PATH_PROGS(BASH, bash)

AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([popt], [poptGetNextOpt], POPT_LIBS="-lpopt",AC_MSG_ERROR([libpopt is required in order to compile LinuxTraceToolkit])  )
#AC_CHECK_LIB([m], [round], M_LIBS="-lm",AC_MSG_ERROR([Mathematical libraries are missing.])  )

AC_CHECK_LIB([util], [forkpty], UTIL_LIBS="-lutil", AC_MSG_ERROR([
libutil is required in order to compile LinuxTraceToolkit]))


# pthread for gdb with dlopen().
AC_CHECK_LIB(pthread, pthread_join, [], AC_MSG_ERROR([LinuxThreads is required in order to make sure gdb works fine with lttv-gui]))

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h pthread.h])

# Set compile flags to java include files if given
AC_ARG_WITH(java_jdk, [  --with-java-jdk=DIR     use java jdk from DIR. Ex : $JAVA_HOME.], JAVA_SDK=$withval,)
if test $JAVA_SDK; then
        if test -d $JAVA_SDK; then
                AC_MSG_RESULT([using java include in $JAVA_SDK])
                SUBDIRS=`find $JAVA_SDK/include -type d`
		CFLAGS+=" "
                CFLAGS+=`for x in $SUBDIRS; do echo -n "-I$x "; done`
		CFLAGS+=" "
        else
                AC_MSG_ERROR(Unable to find java include file in $JAVA_JDK)
        fi
fi


# Check for JNI header files if requested
AC_ARG_WITH(jni-interface, [  --with-jni-interface    build JNI interface between C and java. Need java include files.
			  [[default=no]]])

if test -z "$with_jni_interface"; then
        with_jni_interface=${with_jni_interface_default-no}
fi

if test "$with_jni_interface" = "yes"; then
        AC_CHECK_HEADERS([jni.h],,AC_MSG_ERROR([
missing jni.h
Make sure Sun Java or GJC is installed and that this header file exists in the system path.
Use --with-java-jdk=DIR flag to point to your java include files or desactivate the JNI interface.]))
fi
AM_CONDITIONAL(BUILD_JNI_INTERFACE, test "$with_jni_interface" = "yes")


AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

# If we want the gui, we need additionnal flags for GTK
if test "$with_lttv_gui" = "yes" ; then
        pkg_modules="gtk+-2.0 >= 2.0.0"
        PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
        PACKAGE_CFLAGS="-Wall -Wformat"
        AC_SUBST(PACKAGE_CFLAGS)
        AC_SUBST(PACKAGE_LIBS)
# Else, we still have a dependency on gmodule
# We will add flags for gmodule alone
else
        pkg_modules="gmodule-2.0 >= 2.0.0"
        PKG_CHECK_MODULES(GMODULE, [$pkg_modules])
        PACKAGE_CFLAGS="-Wall -Wformat"
        AC_SUBST(PACKAGE_CFLAGS)
        AC_SUBST(PACKAGE_LIBS)
fi

pkg_modules="gobject-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GOBJECT, [$pkg_modules])

LIBS="$LIBS $GTK_LIBS $GOBJECT_LIBS $GMODULE_LIBS"
PACKAGE_CFLAGS="$GTK_CFLAGS $GOBJECT_CFLAGS $GMODULE_CFLAGS -Wall -Wformat"
MODULE_CFLAGS="$PACKAGE_CFLAGS -fvisibility=hidden"
MODULE_LDFLAGS="-module -avoid-version"
AC_SUBST([PACKAGE_CFLAGS])
AC_SUBST([MODULE_CFLAGS])
AC_SUBST([MODULE_LDFLAGS])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
#AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([select])

#CPPFLAGS="$CPPFLAGS -I"

AC_ARG_ENABLE(lttvstatic,
	AC_HELP_STRING( [--enable-lttvstatic],
		[build a statically linked executable @<:@default=no@:>@]),
		[with_lttvstatic="yes"],
		[with_lttvstatic="no"])
AM_CONDITIONAL(LTTVSTATIC, test "x$with_lttvstatic" = "xyes")
lttvlibdir="${libdir}/lttv"
lttvplugindir="${lttvlibdir}/plugins"
#lttlibdir="${libdir}/ltt"
top_lttvdir="\$(top_srcdir)/lttv"
top_lttvwindowdir="\$(top_srcdir)/lttv/modules/gui/lttvwindow"

DEFAULT_INCLUDES="-I\$(top_srcdir) -I\$(top_builddir) -I\$(top_lttvdir) -I\$(top_lttvwindowdir)"

lttincludedir="${includedir}/ltt"
lttvincludedir="${includedir}/lttv"
lttvwindowincludedir="${includedir}/lttvwindow"
lttctlincludedir="${includedir}/liblttctl"

AC_SUBST([POPT_LIBS])
AC_SUBST([UTIL_LIBS])
AC_SUBST(lttvlibdir)
AC_SUBST(lttvplugindir)
#AC_SUBST(lttlibdir)
AC_SUBST(top_lttvdir)
AC_SUBST(top_lttvwindowdir)
AC_SUBST(DEFAULT_INCLUDES)
AC_SUBST(lttincludedir)
AC_SUBST(lttvincludedir)
AC_SUBST(lttvwindowincludedir)
AC_SUBST(lttctlincludedir)

		 #lttv/modules/gui/tutorial/Makefile
		 #lttv/modules/gui/diskperformance/Makefile
AC_CONFIG_FILES([Makefile
		 lttv/Makefile
		 lttv/lttv/Makefile
		 lttv/modules/Makefile
		 lttv/modules/text/Makefile
		 lttv/modules/gui/Makefile
		 lttv/modules/gui/lttvwindow/Makefile
		 lttv/modules/gui/interrupts/Makefile
		 lttv/modules/gui/lttvwindow/lttvwindow/Makefile
		 lttv/modules/gui/lttvwindow/pixmaps/Makefile
		 lttv/modules/gui/controlflow/Makefile
		 lttv/modules/gui/detailedevents/Makefile
		 lttv/modules/gui/statistics/Makefile
     		 lttv/modules/gui/histogram/Makefile
                 lttv/modules/gui/filter/Makefile
                 lttv/modules/gui/tracecontrol/Makefile
                 lttv/modules/gui/resourceview/Makefile
		 ltt/Makefile
     doc/Makefile
     doc/developer/Makefile
     doc/developer/developer_guide/Makefile
     doc/developer/developer_guide/docbook/Makefile
     doc/developer/developer_guide/html/Makefile
     doc/user/Makefile
     doc/user/user_guide/Makefile
     doc/user/user_guide/docbook/Makefile
     doc/user/user_guide/html/Makefile])
AC_OUTPUT
