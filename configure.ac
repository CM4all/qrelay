AC_PREREQ(2.60)
AC_INIT(CM4all qrelay, 0.4, mk@cm4all.com, cm4all-snowball)
AC_COPYRIGHT(Copyright 2014-2016 Content Management AG)
AC_CONFIG_SRCDIR(src/Main.cxx)
AC_CONFIG_AUX_DIR(build)
AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects silent-rules])
AM_SILENT_RULES

dnl Checks for programs.

AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_CXX

dnl Checks for libraries.

PKG_CHECK_MODULES([LIBEVENT], [libevent >= 2.0.19],,
	[AC_MSG_ERROR([libevent not found])])

PKG_CHECK_MODULES([LIBCM4ALL_DAEMON], [libcm4all-daemon >= 0.4.2],,
	[AC_MSG_ERROR([libcm4all-daemon not found])])

PKG_CHECK_MODULES([LIBCM4ALL_SOCKET], [libcm4all-socket >= 0.3.5],,
	[AC_MSG_ERROR([libcm4all-socket not found])])

PKG_CHECK_MODULES([LIBCM4ALL_INLINE], [libcm4all-inline >= 0.3.4],,
	[AC_MSG_ERROR([libcm4all-inline not found])])

dnl
dnl CFLAGS
dnl

AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_LDFLAGS)

AM_CXXFLAGS="-std=c++14"
AM_CPPFLAGS=""
AM_LDFLAGS="-Wl,--as-needed"

AM_CXXFLAGS="$AM_CXXFLAGS -Wall"
AM_CXXFLAGS="$AM_CXXFLAGS -Wextra"
AM_CXXFLAGS="$AM_CXXFLAGS -Wwrite-strings -Wcast-qual -Wcast-align -Wfloat-equal"
AM_CXXFLAGS="$AM_CXXFLAGS -Wshadow -Wpointer-arith -Wsign-compare"
AM_CXXFLAGS="$AM_CXXFLAGS -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute"
AM_CXXFLAGS="$AM_CXXFLAGS -Wredundant-decls -Wno-long-long -Wundef"
AM_CXXFLAGS="$AM_CXXFLAGS -Wunused"
AM_CXXFLAGS="$AM_CXXFLAGS -pedantic"

dnl
dnl Compile-time options
dnl

AC_ARG_ENABLE(documentation,
	AS_HELP_STRING([--enable-documentation],
		[Enables documentation (default: enabled)]),,
	enable_documentation=yes)
AM_CONDITIONAL(DOCUMENTATION, test x$enable_documentation = xyes)

if test x$enable_documentation = xyes; then
	AC_PATH_PROG([XMLTO], [xmlto], [no])
	if test x$XMLTO = xno; then
		AC_MSG_ERROR([xmlto not found])
	fi
fi

dnl
dnl DEBUG
dnl

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--disable-debug],
		[Disable debugging (default: enabled)]),,
	enable_debug=yes)
AM_CONDITIONAL(TEST, test x$enable_test = xyes)

if test x$enable_debug = xyes; then
	AM_CXXFLAGS="$AM_CXXFLAGS -Werror -pedantic-errors"
else
	AM_CXXFLAGS="$AM_CXXFLAGS -ffunction-sections -fdata-sections"
	AM_CPPFLAGS="$AM_CPPFLAGS -DNDEBUG"
	AM_LDFLAGS="$AM_LDFLAGS -Wl,-gc-sections"
fi

dnl
dnl Output files
dnl

AC_OUTPUT(Makefile)
