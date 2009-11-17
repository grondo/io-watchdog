#define _GNU_SOURCE 1
#include <string.h>       /* GNU basename(3) */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>       /* sleep(3) */


#include "log_msg.h"
#include "io-watchdog.h"

#define GETOPT_ARGS "vn:"

int iterations = 10;

void process_args (int ac, char **av)
{
	int c;

    while ((c = getopt (ac, av, GETOPT_ARGS)) > 0) {
        switch (c) {
        case 'v' :
			log_msg_verbose ();
            break;
		case 'n':
			iterations = atoi (optarg);
			break;
		default:
			log_fatal (3, "Unkown option!\n");
		}
	}
	return;
}

void check_expected_timeout (int expected)
{
	double to;
	/*
	 *  Get timeout:
	 */
	iow_err_t err = io_watchdog_get_timeout (&to);

	if (err != EIOW_SUCCESS)
		log_fatal (1, "io_watchdog_get_timeout: %s\n",
				io_watchdog_strerror (err));

	if ((int) to != expected)
		log_fatal (2, "io_watchdog_get_timeout: timeout = %.3f did not match "
				   "expected timeout of %ds\n", to, expected);
}


int main (int ac, char *av[])
{
	int i;
	double timeout = 1.0;

	log_msg_init (basename (av[0]));

	process_args (ac, av);

	log_verbose ("Stressing io-watchdog API for %d iterations.\n",
			iterations);

	for (i = 0; i < iterations; i++) {
		iow_err_t err = io_watchdog_set_timeout (timeout);

		if (err != EIOW_SUCCESS)
			log_fatal (1, "io_watchdog_set_timeout (%3fs): %s\n",
					timeout, io_watchdog_strerror (err));

		check_expected_timeout ((int) timeout);
		log_verbose ("io_watchdog_set_timeout (%.3fs): Success\n", timeout);
		timeout += 10;
	}

	log_msg_fini ();
	return (0);
}

/*
 *  vi: set ts=4 sw=4 expandtab
 */
