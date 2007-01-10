
#ifndef _HAVE_CONF_H
#define _HAVE_CONF_H
#include "list.h"

typedef struct io_watchdog_conf * io_watchdog_conf_t;

io_watchdog_conf_t io_watchdog_conf_create (void);
void io_watchdog_conf_destroy (io_watchdog_conf_t conf);

int io_watchdog_conf_new_program (io_watchdog_conf_t conf, char *prog);
int io_watchdog_conf_set_timeout (io_watchdog_conf_t conf, char *timeout);
int io_watchdog_conf_set_actions (io_watchdog_conf_t conf, char *actions);
int io_watchdog_conf_set_target (io_watchdog_conf_t conf, char *target);
int io_watchdog_conf_set_rank (io_watchdog_conf_t conf, int rank);
int io_watchdog_conf_append_action_path (io_watchdog_conf_t conf, char *path);

int io_watchdog_conf_set_current_program (io_watchdog_conf_t conf, char *prog);

int          io_watchdog_conf_rank (io_watchdog_conf_t conf);
double       io_watchdog_conf_timeout (io_watchdog_conf_t conf);
const char * io_watchdog_conf_timeout_string (io_watchdog_conf_t conf);
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
