dnl Process this file with autoconf to produce a configure script.
AC_INIT(hengband, 3.0.2.4-Beta)

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS(src/autoconf.h)
AC_CONFIG_SRCDIR(src/main.cpp)
AM_INIT_AUTOMAKE(tar-pax)
AM_SILENT_RULES([yes])

AC_ARG_WITH(setgid,
[  --with-setgid=NAME      install hengband as group NAME],
[case "${withval}" in
  no) GAMEGROUP="";;
  yes) AC_MSG_ERROR(missing argument for --with-setgid);;
  *) GAMEGROUP="$withval";;
esac])
AC_SUBST(GAMEGROUP)
AM_CONDITIONAL(SET_GID, test "$GAMEGROUP" != "")

dnl generate the installation path for the ./lib/ folder
if test "$GAMEGROUP" != ""; then
  MY_EXPAND_DIR(game_libpath, "$datarootdir/games/$PACKAGE/lib/")
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
AC_LANG(C++)
AC_PROG_CXX
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS=$(echo "$CXXFLAGS" | sed 's/-Werror[[^ ]]*//g')
dnl Note that the -std= option for Objective-C++ is hardwired in
dnl src/Makefile.am so that should be changed if this is.
AX_CXX_COMPILE_STDCXX(20, [noext], [mandatory])
CXXFLAGS="$saved_CXXFLAGS"
PKG_PROG_PKG_CONFIG

dnl Detect macOS host
AC_CANONICAL_HOST
is_macos=no
case "${host_os}" in
  darwin*) is_macos=yes ;;
esac

AC_MSG_CHECKING([whether the compiler is Clang])
AC_COMPILE_IFELSE(
  [AC_LANG_SOURCE([[
    #ifdef __clang__
    int main() { return 0; }
    #else
    #error "Not Clang"
    #endif
  ]])],
  [AC_MSG_RESULT([yes]); is_clang=yes],
  [AC_MSG_RESULT([no]); is_clang=no]
)
AM_CONDITIONAL([COMPILER_IS_CLANG], [test "x$is_clang" = "xyes"])

AC_ARG_ENABLE(japanese,
[  --disable-japanese      build english version], use_japanese=no, [AC_DEFINE(JP, 1, [Enable Japanese]) AC_DEFINE(EUC, 1, [Use Extended Unix Code])])

AC_ARG_ENABLE(xim,
[  --disable-xim           disable xim support], use_xim=no, use_xim=yes)
AC_ARG_ENABLE(fontset,
[  --disable-fontset       disable fontset support], use_fontset=no, use_fontset=yes)
AC_ARG_ENABLE([xft],
	AS_HELP_STRING([--enable-xft], [Enable xft support]))
AC_ARG_ENABLE(net,
[  --disable-net           disable networking support], use_net=no)
AC_ARG_ENABLE(worldscore,
[  --disable-worldscore    disable worldscore support], worldscore=no)
AC_ARG_ENABLE([pch],
[  --disable-pch           disable use of precompiled headers],
enable_pch=no, enable_pch=yes)
AM_CONDITIONAL([PCH], [test x$enable_pch = xyes])
AC_ARG_ENABLE([cocoa],
[  --enable-cocoa          enable a Cocoa user interface (OS X only)],
[AC_DEFINE([MACH_O_COCOA], [1], [Use a Cocoa interface (OS X only)])], [])
AM_CONDITIONAL([COCOA], [test x$enable_cocoa = xyes])
dnl Since AC_PROG_OBJCXX has the side effect of setting this conditional,
dnl guarantee that is is always set, regardless of what happens in the if
dnl statement.
AM_CONDITIONAL([am__fastdepOBJCXX], false)
if test x"$enable_cocoa" = xyes ; then
  dnl Use AC_LANG_PUSH and AC_LANG_POP to try to get back to the state that
  dnl AC_LANG_C set in case there are side effects.
  AC_LANG_PUSH([Objective C++])
  AC_PROG_OBJCXX
  AC_LANG_POP([Objective C++])
  AC_DEFINE(SAFE_DIRECTORY, 1, [Mark var directory with version string])
  AC_DEFINE(VERSION_STRING, "3.0.0", [Version string to use for var directory])
fi

dnl Set minimum targeted version of macOS.
AC_ARG_VAR([MACOS_MIN], [minimum version number of macOS to target; default is 10.15 which is the oldest version that the source code can handle])
if test x"$MACOS_MIN" = x ; then
    AC_SUBST(MACOS_MIN_INTERNAL, 10.15)
else
    AC_SUBST(MACOS_MIN_INTERNAL, $MACOS_MIN)
fi

dnl Checks for libraries.
dnl Replace `main' with a function in -lncurses:
AS_IF([test "x$enable_cocoa" != xyes],
  [have_ncurses=no
  curses_lib=
  AC_CHECK_LIB([ncursesw], [initscr],
      [have_ncurses=yes
      curses_lib=ncursesw],
      [AC_CHECK_LIB([ncurses], [initscr],
          [have_ncurses=yes
          curses_lib=ncurses],
          [AC_CHECK_LIB([curses], [initscr],
              [curses_lib=curses],
              [AC_CHECK_LIB([termcap], [tgetent],
                  [AC_DEFINE([USE_CAP], [1], [Allow -mCAP environment])
                  LIBS="$LIBS -ltermcap"])])])])
  AS_IF([test "x$curses_lib" != x], [
      AC_DEFINE([USE_GCU], [1], [Allow -mGCU environment])
      LIBS="$LIBS -l$curses_lib"
      AS_IF([test "x$have_ncurses" = xyes],
          [AC_DEFINE([USE_NCURSES], [1], [Use ncurses])
          dnl Define MACOS_TERMINAL_UTF8 for macOS ncursesw builds
          AS_IF([test "x$is_macos" = "xyes"],
              [AC_DEFINE([MACOS_TERMINAL_UTF8], [1], [Enable UTF-8 width fix for macOS terminal])])])])])

AC_CHECK_LIB(iconv, iconv_open)

if test "$use_net" = no; then
  AC_DEFINE(DISABLE_NET, 1, [Disable networking support])
  worldscore=no;
else
  PKG_CHECK_MODULES(libcurl, [libcurl], , [AC_CHECK_LIB([curl], [curl_global_init], [libcurl_LIBS=-lcurl], [AC_MSG_ERROR([Could not find libcurl for networking support])])])
fi

dnl The world score server is currently only available in Japanese.
if test "$use_japanese" = no; then
  worldscore=no
fi
if test "$worldscore" != no; then
  AC_DEFINE(WORLD_SCORE, 1, [Allow the game to send scores to the score server])
fi

dnl Checks for header files.
dnl AC_PATH_XTRA has side effects on CPP which are beneficial on Mac OS X.
dnl Removing it or conditionally executing it will likely lead to warnings
dnl on that platform about the preprocessor disagreeing with the compiler on
dnl the tests in AC_CHECK_HEADERS.
AC_PATH_XTRA
AS_IF([test "$have_x" = yes && test x"$enable_cocoa" != xyes], [
  LIBS="$LIBS -lX11"
  AC_DEFINE(USE_X11, 1, [Allow -mX11 environment])
  CXXFLAGS="$X_CFLAGS $CXXFLAGS"
  LDFLAGS="$X_LIBS $LDFLAGS"

  if test "$use_fontset" = yes; then
    AC_DEFINE(USE_FONTSET, 1, [Use FontSet])
  else
    if test "$use_japanese" != no; then
      AC_DEFINE(USE_JP_FONTSTRUCT, 1, [Use Kanji without FontSet])
    fi
  fi

  if test "$use_xim" = yes; then
    AC_DEFINE(USE_XIM, 1, [Use X Imput Method])
  fi

  if test "$use_fontset" = yes || test "$use_xim" = yes; then
    AC_DEFINE(USE_LOCALE, 1, [Support Locale])
    if test "$use_japanese" != no; then
      AC_DEFINE(DEFAULT_LOCALE, "ja_JP.eucJP", Default Japanese Locale)
    fi
    AC_CHECK_LIB(X11, _Xsetlocale, AC_DEFINE(X_LOCALE, 1, [Use XLocale]))
    AC_CHECK_LIB(xpg4, setlocale, LIBS="$LIBS -lxpg4")
  fi

  AS_IF([test "x$enable_xft" = "xyes"], [
    PKG_CHECK_MODULES(XFT, [xft],
      [AC_DEFINE([USE_XFT], [1], [Use XFT])])
  ])
])

if test "$use_japanese" != no; then
  AC_MSG_CHECKING([whether the compiler supports -fexec-charset=euc-jp-ms])
  saved_CXXFLAGS="$CXXFLAGS"
  CXXFLAGS="$CXXFLAGS -fexec-charset=euc-jp-ms"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [],
      [return 0;]
    )],
    [AC_MSG_RESULT([yes])
      ac_cv_compile_exec_charset=yes],
    [AC_MSG_RESULT([no])
      ac_cv_compile_exec_charset=no]
  )
  CXXFLAGS="$saved_CXXFLAGS"

  if test x"$ac_cv_compile_exec_charset" = xno; then
    AC_CHECK_PROG(NKF, nkf, yes)
    if test x"$NKF" != x"yes"; then
      AC_MSG_ERROR([nkf is not found. Please install nkf.])
    fi
  fi
fi

AM_CONDITIONAL([COMPILER_HAVE_EXEC_CHARSET_EUC_JP_MS], [test x"$use_japanese" != xno -a x"$ac_cv_compile_exec_charset" = xyes])
AM_CONDITIONAL([USE_NKF], [test x"$use_japanese" != xno -a x"$ac_cv_compile_exec_charset" = xno])

if test "$enable_pch" != no; then
  dnl md5sum, sha224sum, sha256sum, sha384sum, sha512sum and b2sum are
  dnl frequently available on Linux.  md5 is available on macOS and OpenBSD.
  dnl sha1, sha256, and sha512 are available on OpenBSD.
  AC_PATH_PROGS(PCH_CHECKSUMMER, [md5sum md5 sha1sum sha1 sha224sum sha256sum sha256 sha384sum sha512sum sha512 b2sum], [none])
  AC_ARG_VAR([PCH_CHECKSUMMER], [full path to a utility to compute the checksum for the precompiled header; checksum is for ccache's pch_external_checksum])
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
AC_CHECK_FUNCS(gethostname mkdir select socket strtol mkstemp usleep)

dnl On Linux, backtrace() may need additional linker flags to produce useful
dnl symbol names.  With the GNU linker, the necessary option is
dnl --export-dynamic or, if using the compiler to do the linking, -rdynamic.
dnl Use -rdynamic on any platform where it is accepted.  If a platform
dnl requires linking additional libraries to use backtrace(), this may have
dnl to be modified as it only accounts for FreeBSD/NetBSD/OpenBSD where
dnl libexecinfo needs to be linked to have access to backtrace().
AC_SEARCH_LIBS([backtrace], [execinfo],
  [AC_LINK_IFELSE(
    [AC_LANG_SOURCE([[
      #include <execinfo.h>
      int main() { void *a[100]; int n; n = (int)backtrace(a, 100); return (n != 0) ? 0 : 1; }
    ]])],
    [AC_DEFINE([HAVE_BACKTRACE], [1], [Define if backtrace and backtrace_symbols exist.])
    ac_save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -rdynamic"
    AC_LINK_IFELSE(
      [AC_LANG_SOURCE([[
        int main() { return 0; }
      ]])],
      [],
      [LDFLAGS="$ac_save_LDFLAGS"])])])

AC_CONFIG_FILES(Makefile src/Makefile lib/Makefile lib/apex/Makefile \
	lib/bone/Makefile lib/data/Makefile \
	lib/edit/Makefile lib/edit/quests/Makefile lib/edit/towns/Makefile \
	lib/file/Makefile lib/file/books/Makefile lib/help/Makefile \
	lib/info/Makefile lib/pref/Makefile lib/save/Makefile \
	lib/script/Makefile lib/user/Makefile lib/xtra/Makefile \
	lib/xtra/graf/Makefile lib/xtra/music/Makefile lib/xtra/sound/Makefile)
AC_OUTPUT()
