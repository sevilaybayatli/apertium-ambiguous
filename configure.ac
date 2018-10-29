AC_PREREQ(2.52)

AC_INIT([], [0.1.0], [piraye@users.sourceforge.net])


PACKAGE=apertium-ambiguous
VERSION=0.1.0

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_HOST
AC_PROG_CXX
AM_PROG_LIBTOOL
AM_SANITY_CHECK
AC_LANG_CPLUSPLUS


CFLAGS="-Wall -Wextra $CFLAGS"
CXXFLAGS="-Wall -Wextra $CXXFLAGS"

AC_ARG_ENABLE(debug,
              [  --enable-debug  Enable "-g -ggdb -Wall" compiler options],
              [CXXFLAGS="-g -ggdb -Wall";CFLAGS="-g -ggdb -Wall"])

AC_OUTPUT([Makefile src/Makefile])
