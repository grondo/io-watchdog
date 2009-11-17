/*****************************************************************************
 *
 *  Copyright (C) 2007-2008 Lawrence Livermore National Security, LLC.
 *  Produced at Lawrence Livermore National Laboratory.
 *  Written by Mark Grondona <mgrondona@llnl.gov>.
 *
 *  UCRL-CODE-235277
 * 
 *  This file is part of io-watchdog.
 * 
 *  This is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef _LIBIO_WATCHDOG_H
#define _LIBIO_WATCHDOG_H

/*****************************************************************************
 *  C++ macros
 *****************************************************************************/

#undef BEGIN_C_DECLS
#undef END_C_DECLS
#ifdef __cplusplus
#  define BEGIN_C_DECLS         extern "C" {
#  define END_C_DECLS           }
#else  /* !__cplusplus */
#  define BEGIN_C_DECLS         /* empty */
#  define END_C_DECLS           /* empty */
#endif /* !__cplusplus */

/*****************************************************************************
 *  Data types
 *****************************************************************************/

/*
 *  IO Watchdog interface error codes
 */
typedef enum {
    EIOW_SUCCESS     = 0, /*  Success.                                       */
    EIOW_NOT_RUNNING = 1, /*  IO Watchdog not running in this application.   */
    EIOW_BAD_ARG     = 2, /*  Bad argument.                                  */
    EIOW_ERROR       = 3, /*  Generic error.                                 */
} iow_err_t;

const char * io_watchdog_strerror (iow_err_t e);

/*****************************************************************************
 *  IO Watchdog application interface
 *****************************************************************************/

BEGIN_C_DECLS

/*
 *  Set io-watchdog timeout in seconds.
 */
iow_err_t io_watchdog_set_timeout (double timeout); 

/*
 *  Get current io-watchdog timeout in seconds.
 */
iow_err_t io_watchdog_get_timeout (double *tp); 

END_C_DECLS

#endif /* !_LIBIO_WATCHDOG_H */
