AC_INIT([dcstad], [0.1], [j.dennis@cablelabs.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
#AC_PROG_CPP

AC_CHECK_HEADERS([dcwsocket.h], [], [
  AC_MSG_ERROR([unable to find the dcwsocket headers])
])
AC_SEARCH_LIBS([dcwsock_open], [dcwsocket], [], [
  AC_MSG_ERROR([unable to find the dcwsocket library])
])


AC_CHECK_HEADERS([dcwproto.h], [], [
  AC_MSG_ERROR([unable to find the dcwproto headers])
])
AC_SEARCH_LIBS([dcwmsg_marshal], [dcwproto], [], [
  AC_MSG_ERROR([unable to find the dcwproto library])
])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
#AC_CONFIG_MACRO_DIR([m4])

#LT_INIT
#AC_ENABLE_SHARED
#AC_ENABLE_STATIC
#AC_PROG_LIBTOOL

AC_OUTPUT
