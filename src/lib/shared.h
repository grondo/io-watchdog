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

struct io_watchdog_shared_info {
	pthread_mutex_t    mutex;
	pthread_cond_t     cond;
	unsigned int       barrier:1;
	unsigned int       flag:1;
	unsigned int       exited:1;
	struct timeval     lastio;
	unsigned long long nbytes;
	time_t             start_time; 
	pid_t              monitored_pid;
	pid_t              server_pid;
	char               cmd [64];
};

struct io_watchdog_shared_region {
	int fd;
	char * path;
	struct io_watchdog_shared_info *shared;
};

struct io_watchdog_shared_region *io_watchdog_shared_region_create (char *file);
void io_watchdog_shared_region_destroy (struct io_watchdog_shared_region *s);
int io_watchdog_shared_info_barrier (struct io_watchdog_shared_info *s);

#endif /* !_SHARED_H */
