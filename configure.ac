#******************************************************************************
# Copyright (C) 2007-2008 Lawrence Livermore National Security, LLC.
# Produced at Lawrence Livermore National Laboratory.
# Written by Mark Grondona <mgrondona@llnl.gov>.
#
# UCRL-CODE-235277
#
# This file is part of io-watchdog.
#
# This is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#****************************************************************************** 
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
#
# Checks for headers
#
AC_CHECK_HEADERS(slurm/spank.h)
AM_CONDITIONAL(WITH_SPANK, test x"${ac_cv_header_slurm_spank_h}" != xno)
#
#
#
X_AC_CHECK_PTHREADS

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

