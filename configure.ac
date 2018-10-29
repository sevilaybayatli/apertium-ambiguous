AC_PREREQ(2.52)


CFLAGS="-Wall -Wextra $CFLAGS"
CXXFLAGS="-Wall -Wextra $CXXFLAGS"

AC_ARG_ENABLE(debug,
              [  --enable-debug  Enable "-g -ggdb -Wall" compiler options],
              [CXXFLAGS="-g -ggdb -Wall";CFLAGS="-g -ggdb -Wall"])

AC_OUTPUT([Makefile src/Makefile])
