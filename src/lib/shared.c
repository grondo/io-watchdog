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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


#include "log_msg.h"
#include "shared.h"


static char * shared_file_name_create (char *file)
{
    char *tmpdir;
    char buf [4096];

    if (file) return (strdup (file));

    if (!(tmpdir = getenv ("TMPDIR"))) 
        tmpdir = "/tmp";

    srand (getpid ());

    snprintf (buf, sizeof (buf), "%s/io-watchdog-%d-%04x", 
             tmpdir, (int) getpid (), rand ());

    return (strdup (buf));
}

static int shared_region_init (struct io_watchdog_shared_region *s)
{
    int err;
    pthread_mutexattr_t   m_attr;
    pthread_condattr_t    c_attr;

    memset (s->shared, 0, sizeof (*s->shared));

    err = pthread_mutexattr_init (&m_attr);
    if (err != 0) {
        log_err ("pthread_mutexattr_init: %s\n", strerror (err));
        return (-1);
    }

    err = pthread_mutexattr_setpshared (&m_attr, PTHREAD_PROCESS_SHARED);
    if (err != 0) {
        log_err ("pthread_mutexattr_setpshared: %s\n", strerror (err));
        return (-1);
    }

    err = pthread_mutex_init (&s->shared->mutex, &m_attr);
    if (err != 0) {
        log_err ("pthread_mutex_init: %s\n", strerror (err));
        return (-1);
    }

    err = pthread_condattr_init (&c_attr);
    if (err != 0) {
        log_err ("pthread_condattr_init: %s\n", strerror (err));
        return (-1);
    }

    err = pthread_condattr_setpshared (&c_attr, PTHREAD_PROCESS_SHARED);
    if (err != 0) {
        log_err ("pthread_condattr_setpshared: %s\n", strerror (err));
        return (-1);
    }

    err = pthread_cond_init (&s->shared->cond, &c_attr);
    if (err != 0) {
        log_err ("pthread_cond_init: %s\n", strerror (err));
        return (-1);
    }

    return (0);
}

struct io_watchdog_shared_region *
io_watchdog_shared_region_create (char *file)
{
    int flags = O_RDWR | O_CREAT | O_EXCL;
    int first = 0;

    struct io_watchdog_shared_region * s = malloc (sizeof (*s));
    int len = sizeof (*s->shared);

    if (s == NULL)
        return (NULL);

    s->path = shared_file_name_create (file);
    s->fd = -1;

    if ((s->fd = open (s->path, flags, 0600)) >= 0) {
        first = 1;
        ftruncate (s->fd, len);
    }
    else if ((errno != EEXIST) || ((s->fd = open (s->path, O_RDWR)) < 0)) {
        log_err ("Failed to open `%s': %s\n", s->path, strerror (errno));
        io_watchdog_shared_region_destroy (s);
        return (NULL);
    }

    s->shared = mmap (0, len, PROT_READ|PROT_WRITE, MAP_SHARED, s->fd, 0);
    if (s->shared == MAP_FAILED) {
        log_err ("mmap (%s): %s\n", s->path, strerror (errno));
        io_watchdog_shared_region_destroy (s);
        return (NULL);
    }

    if (first)
        shared_region_init (s);
    else
        unlink (s->path);

    if (fcntl (s->fd, F_SETFD, FD_CLOEXEC) < 0)
        log_err ("Failed to set close-on-exec flag for fd %d: %s\n", s->fd, 
                strerror (errno));

    log_verbose ("shared file [%s] fd %d\n", s->path, s->fd);

    return (s);

}

int io_watchdog_shared_info_barrier (struct io_watchdog_shared_info *s)
{
    pthread_mutex_lock (&s->mutex);
    if (s->barrier) {
        s->barrier = 0;
        pthread_cond_signal (&s->cond);
    }
    else {
        s->barrier = 1;
        pthread_cond_wait (&s->cond, &s->mutex);
    }
    pthread_mutex_unlock (&s->mutex);

    return (0);
}

void io_watchdog_shared_region_destroy (struct io_watchdog_shared_region *s)
{
    if (!s) return;

    if (s->path)
        free (s->path);
    if (s->fd >= 0)
        close (s->fd);
    s->shared = NULL;
    free (s);
}

/*
 * vi: ts=4 sw=4 expandtab
 */
