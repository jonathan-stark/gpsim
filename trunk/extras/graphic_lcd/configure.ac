#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([graphicLCD], [0.1.0], [<URL:http://www.dattalo.com/gnupic/gpsim.html>])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE


AC_PATH_PROG(PKGCONFIG, pkg-config, no, ${PATH})

if test $PKGCONFIG = "no"; then
   AC_MSG_ERROR(Cannot find pkg-config)
fi

X_LDFLAGS=`$PKGCONFIG --libs gtk+-2.0 gthread-2.0`
X_CFLAGS=`$PKGCONFIG --cflags gtk+-2.0`

AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_LANG([C++])
AC_CHECK_LIB([gpsim], [main])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/file.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_SIGNAL

# Checks for library functions.
AC_FUNC_ALLOCA
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor gethostbyname gethostname gettimeofday memset pow select socket sqrt strcasecmp strchr strdup strerror strncasecmp strndup strpbrk strrchr strstr strtoul])

# printf modifier define for long long as "ll"
# config_win32.h.in defines this for Visual Studio stdclib as "I64"
AC_DEFINE([PRINTF_INT64_MODIFIER],["ll"],[printf modifier define for long long])


AM_CFLAGS=
AM_CXXFLAGS=
AM_LDFLAGS=

# Options for the system on which the package will run
case "${host}" in
  *linux* )
    if test "x$GCC" = "xyes"; then
      AM_CFLAGS="-Wall"
      AM_CXXFLAGS="-Wall"
      AM_LDFLAGS="-Wl,-warn-common -Wl,-warn-once"
    fi
    ;;
  *mingw* )
    ;;
esac

# Host filesystem options
case "${host}" in
  *mingw* | *-pc-os2_emx | *-pc-os2-emx | *djgpp* )
    AC_DEFINE(HAVE_DOS_BASED_FILE_SYSTEM, 1, 
      [Define if your host uses a DOS based file system. ])
    ;;
esac

AC_SUBST(Y_CFLAGS)
AC_SUBST(Y_LDFLAGS)
AC_SUBST(X_CFLAGS)
AC_SUBST(X_LDFLAGS)
AC_SUBST(GTK)
AC_SUBST(GDK)
AC_SUBST(GLIB)
AC_SUBST(LIBREADLINE)

AC_CONFIG_FILES([Makefile
		 src/Makefile
                 ])

AC_OUTPUT


AC_MSG_RESULT([

graphicLCD-$PACKAGE_VERSION is now configured for $canonical_host_type

  Build:                $build
  Host:                 $host
  Source directory:     $srcdir
  Installation prefix:  $prefix
  C compiler:           $CC $CPPFLAGS $CFLAGS
  C++ compiler:         $CXX $CPPFLAGS $CXXFLAGS

])
