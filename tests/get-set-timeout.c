
#define _GNU_SOURCE 1
#include <string.h>       /* GNU basename(3) */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>       /* sleep(3) */


#include "log_msg.h"
#include "io-watchdog.h"

#define GETOPT_ARGS "vt:"

int expected_timeout = 0;

void process_args (int ac, char **av)
{
	int c;

    while ((c = getopt (ac, av, GETOPT_ARGS)) > 0) {
        switch (c) {
        case 'v' :
			log_msg_verbose ();
            break;
		case 't':
			expected_timeout = atoi (optarg);
			break;
		default:
			log_fatal (3, "Unkown option!\n");
		}
	}
	return;
}

void check_expected_timeout (double *tp)
{
	/*
	 *  Get timeout:
	 */
	iow_err_t err = io_watchdog_get_timeout (tp);

	if (err != EIOW_SUCCESS)
		log_fatal (1, "io_watchdog_get_timeout: %s\n",
				io_watchdog_strerror (err));

	if ((int) *tp != expected_timeout)
		log_fatal (2, "io_watchdog_get_timeout: timeout = %.3f did not match "
				   "expected timeout of %ds\n", *tp, expected_timeout);
}


int main (int ac, char *av[])
{
	double timeout;
	iow_err_t err;

	log_msg_init (basename (av[0]));

	process_args (ac, av);

	check_expected_timeout (&timeout);

	log_verbose ("io_watchdog_get_timeout: Success\n");

	timeout += 10;
	log_verbose ("Setting watchdog timeout to %.3fs\n", timeout);

	err = io_watchdog_set_timeout (timeout);

	if (err != EIOW_SUCCESS)
		log_fatal (1, "io_watchdog_set_timeout (%3fs): %s\n",
				timeout, io_watchdog_strerror (err));

	expected_timeout = (int) timeout;
	check_expected_timeout (&timeout);

	log_verbose ("io_watchdog_set_timeout: Success\n");

	log_msg_fini ();
	return (0);
}

/*
 *  vi: set ts=4 sw=4 expandtab
 */
