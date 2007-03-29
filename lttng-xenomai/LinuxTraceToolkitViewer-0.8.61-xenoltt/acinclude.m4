# AC_LTTV_SETUP_FOR_TARGET
# ----------------
# Check for utils function
### [AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_DEFUN([AC_LTTV_SETUP_FOR_TARGET],
[ UTIL_LIBS=""
case $host in
*-*-cygwin*)
  AC_SUBST(SHARED_FLAGS,"-module -no-undefined -XLinker --export-all-symbols")
  AC_SUBST(PLUGIN_FLAGS,"-module -no-undefined -avoid-version")
  AC_CHECK_LIB([util], [forkpty], UTIL_LIBS="-lutil", AC_MSG_ERROR([libutil is required in order to compile LinuxTraceToolkit]))
  cygwin=true
  ;;

*-*-mingw*)
  # mingw32 doesn't have libutil
  AC_SUBST(SHARED_FLAGS,"-module -no-undefined -avoid-version -XLinker --export-all-symbols")
  AC_SUBST(PLUGIN_FLAGS,"-module -no-undefined -avoid-version")
  mingw=true
  ;;
*)
  AC_SUBST(SHARED_FLAGS,"-module")
  AC_SUBST(PLUGIN_FLAGS,"-module -avoid-version")
  AC_CHECK_LIB([util], [forkpty], UTIL_LIBS="-lutil", AC_MSG_ERROR([libutil is required in order to compile LinuxTraceToolkit]))
  linux=true
  ;;
esac
AM_CONDITIONAL(LTTV_MINGW, test x$mingw = xtrue)
AM_CONDITIONAL(LTTV_CYGWIN, test x$cygwin = xtrue)
AM_CONDITIONAL(LTTV_LINUX, test x$linux = xtrue)
AM_CONDITIONAL(USE_UTIL_LIBS, test "$UTIL_LIBS" != "")
])# AC_LTTV_SETUP_FOR_TARGET
