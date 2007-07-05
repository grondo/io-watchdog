#ifndef _SHARED_H
#define _SHARED_H

#include <sys/types.h>
#include <sys/time.h>

struct io_watchdog_shared_info {
	unsigned int       flag;
	struct timeval     lastio;
	unsigned int       started;
	unsigned int       exited;
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

#endif /* !_SHARED_H */
