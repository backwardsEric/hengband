dnl Process this file with autoconf to produce a configure script.
AC_INIT(hengband, 1.6.2)

AC_CONFIG_HEADERS(src/autoconf.h)
AC_CONFIG_SRCDIR(src/main.c)
AM_INIT_AUTOMAKE

AC_ARG_WITH(setgid,
[  --with-setgid=NAME      install hengband as group NAME],
[case "${withval}" in
  no) GAMEGROUP="";;
  yes) AC_MSG_ERROR(missing argument for --with-setgid);;
  *) GAMEGROUP="$withval";;
esac])
AC_SUBST(GAMEGROUP)
AM_CONDITIONAL(SET_GID, test "$GAMEGROUP" != "")

AC_LANG([C])

dnl generate the installation path for the ./lib/ folder
if test "$GAMEGROUP" != ""; then
  MY_EXPAND_DIR(game_libpath, "$datadir/games/$PACKAGE/lib/")
else
  MY_EXPAND_DIR(game_libpath, "./lib/")
  bindir="`pwd`"
fi

dnl overwrite the path with an user-specified value
AC_ARG_WITH(libpath,
[  --with-libpath=path     specify the path to the Hengband lib folder],
[game_libpath="$withval"])

AC_ARG_WITH(varpath,
[  --with-varpath=path     specify the path to the Hengband var folder],
[game_varpath="$withval"],
[game_varpath="$game_libpath"])

AC_DEFINE_UNQUOTED(DEFAULT_LIB_PATH, "$game_libpath", [Path to the Hengband lib folder])
DEFAULT_LIB_PATH="$game_libpath"
AC_SUBST(DEFAULT_LIB_PATH)

AC_DEFINE_UNQUOTED(DEFAULT_VAR_PATH, "$game_varpath", [Path to the Hengband var folder])
DEFAULT_VAR_PATH="$game_varpath"
AC_SUBST(DEFAULT_VAR_PATH)

dnl Checks for programs.
AC_PROG_CC

AC_ARG_ENABLE(japanese,
[  --disable-japanese      build english version], use_japanese=no, [AC_DEFINE(JP, 1, [Enable Japanese]) AC_DEFINE(EUC, 1, [Use Extended Unix Code])])

AC_ARG_ENABLE(xim,
[  --disable-xim           disable xim support], use_xim=no, use_xim=yes)
AC_ARG_ENABLE(fontset,
[  --disable-fontset       disable fontset support], use_fontset=no, use_fontset=yes)
AC_ARG_ENABLE(worldscore,
[  --disable-worldscore    disable worldscore support], ,AC_DEFINE(WORLD_SCORE, 1, [Allow the game to send scores to the score server]))
AC_ARG_ENABLE(chuukei,
[  --enable-chuukei        enable internet chuukei support], AC_DEFINE(CHUUKEI, 1, [Chuukei mode]))
AC_ARG_ENABLE([cocoa],
[  --enable-cocoa          enable a Cocoa user interface (OS X only)],
AC_DEFINE([MACH_O_COCOA], [1], [Use a Cocoa interface (OS X only)]), [])
AM_CONDITIONAL([COCOA], [test x$enable_cocoa = xyes])
dnl Since AC_PROG_OBJC has the side effect of setting this conditional,
dnl guarantee that is is always set, regardless of what happens in the if
dnl statement.
AM_CONDITIONAL([am__fastdepOBJC], false)
if test x"$enable_cocoa" = xyes ; then
  dnl Use AC_LANG_PUSH and AC_LANG_POP to try to get back to the state that
  dnl AC_LANG_C set in case there are side effects.
  AC_LANG_PUSH([Objective C])
  AC_PROG_OBJC
  AC_LANG_POP([Objective C])
  AC_DEFINE(USE_TRANSPARENCY, 1, [Use transparency mode])
  AC_DEFINE(SAFE_DIRECTORY, 1, [Mark var directory with version string])
  AC_DEFINE(VERSION_STRING, "1.6.2", [Version string to use for var directory])
fi

dnl Set minimum targeted version of macOS.
AC_ARG_VAR([MACOS_MIN], [minimum version number of macOS to target; default is 10.11 for compatibility with XCode 15; the oldest feasible version is 10.8])
if test x"$MACOS_MIN" = x ; then
    AC_SUBST(MACOS_MIN_INTERNAL, 10.11)
else
    AC_SUBST(MACOS_MIN_INTERNAL, $MACOS_MIN)
fi

dnl -fno-strength-reduce prevents a bug in some versions of gcc
if test "$GCC" = yes; then
  AC_MSG_CHECKING([if gcc supports -fno-strength-reduce])
  _gcc_cflags_save=$CFLAGS
  CFLAGS="$CFLAGS -fno-strength-reduce -Werror"
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([])],_gcc_stropt=yes,_gcc_stropt=no)
  AC_MSG_RESULT($_gcc_stropt)
  CFLAGS=$_gcc_cflags_save
  if test x"$_gcc_stropt" = xyes ; then
      CFLAGS="$CFLAGS -fno-strength-reduce"
  fi
fi

dnl Checks for libraries.
dnl Replace `main' with a function in -lncurses:
AC_CHECK_LIB(ncurses, initscr, [AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) AC_DEFINE(USE_NCURSES, 1, [Use ncurses]) LIBS="$LIBS -lncurses"])
if test "$ac_cv_lib_ncurses_initscr" != yes; then
  AC_CHECK_LIB(curses, initscr, [AC_DEFINE(USE_GCU, 1, [Allow -mGCU environment]) LIBS="$LIBS -lcurses"])
  if test "$ac_cv_lib_curses_initscr" != yes; then
    AC_CHECK_LIB(termcap, tgetent, [AC_DEFINE(USE_CAP, 1, [Allow -mCAP environment]) LIBS="$LIBS -ltermcap"])
  fi
fi

dnl Checks for header files.
dnl AC_PATH_XTRA has side effects on CPP which are beneficial on Mac OS X.
dnl Removing it or conditionally executing it will likely lead to warnings
dnl on that platform about the preprocessor disagreeing with the compiler on
dnl the tests in AC_CHECK_HEADERS.
AC_PATH_XTRA
if test "$have_x" = yes && test x"$enable_cocoa" != xyes ; then
  LIBS="$LIBS -lX11"
  AC_DEFINE(USE_X11, 1, [Allow -mX11 environment])
  AC_DEFINE(USE_TRANSPARENCY, 1, [Use transparency mode])
  CFLAGS="$X_CFLAGS $CFLAGS"
  LDFLAGS="$X_LIBS $LDFLAGS"
  if test "$use_japanese" != no; then
    if test "$use_fontset" = yes; then
      AC_DEFINE(USE_FONTSET, 1, [Use FontSet])
    else
      AC_DEFINE(_JP, 1, [Use Kanji without FontSet])
    fi
    if test "$use_xim" = yes; then
      AC_DEFINE(USE_XIM, 1, [Use X Imput Method])
    fi
    if test "$use_fontset" = yes || test "$use_xim" = yes; then
      AC_DEFINE(USE_LOCALE, 1, [Support Locale])
      AC_DEFINE(DEFAULT_LOCALE, "ja_JP.eucJP", Default Japanese Locale)
      AC_CHECK_LIB(X11, _Xsetlocale, AC_DEFINE(X_LOCALE, 1, [Use XLocale]))
      AC_CHECK_LIB(xpg4, setlocale, LIBS="$LIBS -lxpg4")
    fi
  fi
fi

AC_CHECK_HEADERS(fcntl.h sys/file.h sys/ioctl.h sys/time.h termio.h unistd.h stdint.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gethostname mkdir select socket strstr strtol vasprintf vsnprintf mkstemp usleep)

AC_CONFIG_FILES(Makefile src/Makefile lib/Makefile lib/apex/Makefile lib/bone/Makefile lib/data/Makefile lib/edit/Makefile lib/file/Makefile lib/help/Makefile lib/info/Makefile lib/pref/Makefile lib/save/Makefile lib/script/Makefile lib/user/Makefile lib/xtra/Makefile lib/xtra/font/Makefile lib/xtra/graf/Makefile lib/xtra/music/Makefile lib/xtra/sound/Makefile)
AC_OUTPUT()
