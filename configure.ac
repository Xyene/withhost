AC_INIT([withhost], [0.1], [me@tbrindus.ca])
AM_INIT_AUTOMAKE
LT_INIT
AC_PROG_CC
AC_CONFIG_FILES([Makefile])
AC_SEARCH_LIBS([dlopen], [dl dld], [], [
    AC_MSG_ERROR([unable to find dlopen()])
])
AC_OUTPUT
