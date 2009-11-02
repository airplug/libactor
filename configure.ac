AC_INIT([libactor],[0.1],[chris@tech9computers.com])

AC_CONFIG_SRCDIR([src/actor.c])

AC_PROG_CC

AM_INIT_AUTOMAKE([1.9])

AC_PROG_LIBTOOL
AC_TYPE_SIZE_T
AC_CHECK_LIB([pthread], [pthread_create],,AC_ERROR(pthread library not found))
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile examples/Makefile doc/Makefile])
CFLAGS="-Wall"
AC_OUTPUT