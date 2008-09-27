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
#ifndef _HAVE_CONF_H
#define _HAVE_CONF_H
#include "list.h"

typedef struct io_watchdog_conf * io_watchdog_conf_t;

const char * io_watchdog_server_path ();

io_watchdog_conf_t io_watchdog_conf_create (void);
void io_watchdog_conf_destroy (io_watchdog_conf_t conf);

int io_watchdog_conf_new_program (io_watchdog_conf_t conf, char *prog);
int io_watchdog_conf_set_timeout (io_watchdog_conf_t conf, char *timeout);
int io_watchdog_conf_set_exact_timeout (io_watchdog_conf_t conf, int exact);
int io_watchdog_conf_set_actions (io_watchdog_conf_t conf, char *actions);
int io_watchdog_conf_set_target (io_watchdog_conf_t conf, char *target);
int io_watchdog_conf_set_rank (io_watchdog_conf_t conf, int rank);
int io_watchdog_conf_append_action_path (io_watchdog_conf_t conf, char *path);

int io_watchdog_conf_set_current_program (io_watchdog_conf_t conf, char *prog);

int          io_watchdog_conf_rank (io_watchdog_conf_t conf);
double       io_watchdog_conf_timeout (io_watchdog_conf_t conf);
const char * io_watchdog_conf_timeout_string (io_watchdog_conf_t conf);
int          io_watchdog_conf_exact_timeout (io_watchdog_conf_t conf);
int          io_watchdog_conf_timeout_has_suffix (io_watchdog_conf_t conf);
const char * io_watchdog_conf_target (io_watchdog_conf_t conf);
List         io_watchdog_conf_actions (io_watchdog_conf_t conf);

int io_watchdog_conf_append_action (io_watchdog_conf_t conf, 
		                    char *name, char *path);

int io_watchdog_conf_parse (io_watchdog_conf_t conf, const char *filename);
int io_watchdog_conf_parse_defaults (io_watchdog_conf_t conf);
int io_watchdog_conf_parse_system (io_watchdog_conf_t conf);
int io_watchdog_conf_parse_user (io_watchdog_conf_t conf, const char *path);

int io_watchdog_conf_list_actions (io_watchdog_conf_t conf);
const char * io_watchdog_conf_find_action (io_watchdog_conf_t conf, char *name);

void io_watchdog_conf_debug ();

int parse_timeout_string (const char *s, double *dp, int *has_suffix);


#endif /* !_HAVE_CONF_H */
