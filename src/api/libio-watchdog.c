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

/*
 *  This library is a stub.
 *  The actual io-watchdog interface is provided by the interposer
 *   library symbols which will override these symbols when applications
 *   run under the io-watchdog. Therefore, these functions all return
 *   IOW_ENOSYS
 */

#include "io-watchdog.h"

iow_err_t io_watchdog_get_timeout (double *tp)
{
	return EIOW_NOT_RUNNING;
}

iow_err_t io_watchdog_set_timeout (double to)
{
	return EIOW_NOT_RUNNING;
}

const char * io_watchdog_strerror (iow_err_t err)
{
	switch (err) {
		case EIOW_SUCCESS:
			return "Success";
		case EIOW_NOT_RUNNING:
			return "No io-watchdog server attached to this process";
		case EIOW_BAD_ARG:
			return "Invalid argument";
		default:
			return "Unspecified error";
	}
}
