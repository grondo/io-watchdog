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
#ifndef _SHARED_H
#define _SHARED_H

#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

enum client_req {
	IO_REQ_NONE = 0,
	IO_REQ_GET_TIMEOUT,
	IO_REQ_SET_TIMEOUT
};

struct io_watchdog_shared_info {
	pthread_mutex_t    mutex;           /*  Process-shared mutex              */
	pthread_cond_t     cond;            /*  Process-shared condition var      */
	unsigned int       barrier:1;       /*  Flag for interprocess barrier     */
	unsigned int       flag:1;          /*  Flag for client write activity    */
	unsigned int       exited:1;        /*  True if client has exited         */
	struct timeval     lastio;          /*  Timestamp of last IO              */
	unsigned long long nbytes;          /*  Count of bytes written            */
	time_t             start_time;      /*  Client process start time         */
	pid_t              monitored_pid;   /*  PID of client process             */
	pid_t              server_pid;      /*  PID of io-watchdog server process */
	char               cmd [64];        /*  Short command name of client      */

	/*
	 *  Client API members:
	 */
	enum client_req   req_type;         /*  Type of request from client       */

	union {                             /*  Union of possible request data    */
		double timeout;                 /*  io-watchdog timeout               */
		char   data [256];              /*  Placeholder for future data       */
	} info;
};

struct io_watchdog_shared_region {
	int fd;
	char * path;
	struct io_watchdog_shared_info *shared;
};

struct io_watchdog_shared_region *io_watchdog_shared_region_create (char *file);
void io_watchdog_shared_region_destroy (struct io_watchdog_shared_region *s);

int io_watchdog_shared_info_barrier (struct io_watchdog_shared_info *s);

int
io_watchdog_shared_get_timeout (struct io_watchdog_shared_info *s, double *to);

int
io_watchdog_shared_set_timeout (struct io_watchdog_shared_info *s, double to);


#endif /* !_SHARED_H */
