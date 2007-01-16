#
# This file must be processed with autoconf to generate a configure script
#

AC_INIT
X_AC_META
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([src/watchdog/io-watchdog.c])
AC_CANONICAL_HOST

#
#  Automake support
# 
AM_INIT_AUTOMAKE([$PACKAGE], [$VERSION])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE
#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_INSTALL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AM_PROG_LEX

CFLAGS="-g -Wall"

AC_CONFIG_FILES([
   Makefile
   src/Makefile
   src/lib/Makefile
   src/conf/Makefile
   src/conf/io-watchdog.conf.5
   src/spank/Makefile
   src/watchdog/Makefile
   src/watchdog/io-watchdog.1
   src/interposer/Makefile
 ]
)

AC_OUTPUT

