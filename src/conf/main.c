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
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "conf.h"
#include "log_msg.h"

static char *file = "-";

int parse_options (int ac, char **av)
{
	int c;

	while ((c = getopt (ac, av, "f:d")) != -1) {
		switch (c) {
		case 'f': 
			file = optarg; 
			break;
		case 'd':
			io_watchdog_conf_debug ();
			break;
		default:
			log_err ("Invalid option: -%c\n", c);
			break;
		}
	}
	return (0);
}

char * print_list (List l)
{
	ListIterator i = list_iterator_create (l);
	char *s;
	static char buf [4096];

	memset (buf, 0, sizeof (buf));

	while ((s = list_next (i))) {
		if (strlen (buf)) strcat (buf, ", ");
		strcat (buf, s);
	}

	list_iterator_destroy (i);

	return (buf);
}

int main (int ac, char **av)
{
	io_watchdog_conf_t conf = io_watchdog_conf_create ();

	log_msg_init ("test");
	log_msg_set_verbose (3);

	parse_options (ac, av);

	if (io_watchdog_conf_parse (conf, file) < 0)
		log_fatal (1, "%s: Parsing failed\n");

	log_msg ("timeout = %s, rank = %d, actions = [%s], target = %s\n",
			io_watchdog_conf_timeout_string (conf),
			io_watchdog_conf_rank (conf),
			print_list (io_watchdog_conf_actions (conf)),
			io_watchdog_conf_target (conf));

	io_watchdog_conf_set_current_program (conf, "/bin/sleep");

	log_msg ("timeout = %s, rank = %d, actions = [%s], target = %s\n",
			io_watchdog_conf_timeout_string (conf),
			io_watchdog_conf_rank (conf),
			print_list (io_watchdog_conf_actions (conf)),
			io_watchdog_conf_target (conf));


	io_watchdog_conf_destroy (conf);

	return (0);
}
