
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
