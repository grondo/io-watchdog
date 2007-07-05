#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <slurm/spank.h>

#include "split.h"
#include "list.h"
#include "conf.h"


SPANK_PLUGIN (io-watchdog, 1);

struct io_watchdog_options {
    unsigned int enabled:1;

    int  rank;
    int  exact_timeout;
    char *action;
    char *timeout;
    char *debug;
    char *target;
    char *conf;
    
    char *argv0;
    char *shared_filename;
};

static char io_watchdog_help [] = 
"io-watchdog: Usage: --io-watchdog=[arg1,arg2,...]\n\
  conf=file            Read config from [file] instead of ~/.io-watchdogrc\n\
  rank=N               Run watchdog on rank N (default = 0).\n\
  timeout=N[suffix]    Set watchdog timeout to N seconds. Suffix may be\n\
                       `s' for seconds (the default), `m' for minutes,\n\
                       `h' for hours, or `d' for days. N may be an arbitrary\n\
                       floating point number. (default = 1h)\n\
  exact[-timeout]      Use a more precise method for the watchdog timeout.\n\
                       See the io-watchdog(1) man page for more information.\n\
  action=script        Run `script' if timeout is reached without any writes \n\
                       from rank N. (default = no action)\n\
  target=glob          Only target processes with names matching the shell\n\
                       globbing pattern `glob' (See glob(7)). This is useful\n\
                       when running jobs under other commands like time(1)\n\
                       so that the io-watchdog is not run on the time(1)\n\
                       process. (default = target all processes)\n\
  debug=N              Set io watchdog debug level to N.\n\
  help                 Display this help message.\n";


static struct io_watchdog_options opts = {
    .enabled = 0,
    .rank    = -1,
    .action  = NULL,
    .timeout = NULL,
    .target  = NULL,
    .conf    = NULL,
    .argv0   = NULL
};

static int io_watchdog_opt_process (int val, const char *optarg, int remote);

struct spank_option spank_options [] = 
{
    { "io-watchdog", "[args..]", 
      "Use io-watchdog service for this job (args=`help' for more info)",
      2, 1, (spank_opt_cb_f) io_watchdog_opt_process
    },
    SPANK_OPTIONS_TABLE_END
};

static int str2uint (const char *str, unsigned int *p2uint)
{
    long int l;
    char *p;

    l = strtol (str, &p, 10);
    if ((*p != '\0') || (l < 0))
        return (-1);

    *p2uint = (unsigned int) l;

    return (0);
}

static int valid_timeout_string (const char *to)
{
    double d;
    int has_units;

    if (parse_timeout_string (to, &d, &has_units) < 0) {
        slurm_error ("io-watchdog: Invalid timeout string `%s'", to);
        return (0);
    }

    return (1);
}

static int handle_watchdog_arg (char *arg, void *unused)
{
    if (strncmp (arg, "rank=", 5) == 0) {
        if (str2uint (arg+5, &opts.rank) < 0) {
            slurm_error ("io-watchdog: Invalid rank `%s'\n", arg+5);
            return (-1);
        }
    } 
    else if (strncmp (arg, "timeout=", 8) == 0) {
        opts.timeout = strdup (arg+8);
        if (!valid_timeout_string (opts.timeout)) 
            return (-1);
    }
    else if (  strncmp (arg, "exact-timeout", 13) == 0
            || strncmp (arg, "exact", 5) == 0 )  {
        opts.exact_timeout = 1;
    }
    else if (strncmp (arg, "action=", 7) == 0) { 
        opts.action = strdup (arg+7);
    }
    else if (strncmp (arg, "target=", 7) == 0) {
        opts.target = strdup (arg+7);
    }
    else if (strncmp (arg, "debug=", 6) == 0) {
        opts.debug = strdup (arg+6);
    }
    else if (strncmp (arg, "conf=", 5) == 0) {
        opts.conf = strdup (arg+5);
    }
    else if (strcmp (arg, "help") == 0)
        slurm_info (io_watchdog_help);
    else {
        slurm_error ("io-watchdog: Invalid argument `%s'", arg);
        return (-1);
    }

    return (0);
}

static int parse_watchdog_args (const char *args)
{
    int rc;
    char buf [4096];

    strncpy (buf, args, sizeof (buf));

    List l = list_split (",", buf);
    rc = list_for_each (l, (ListForF) handle_watchdog_arg, NULL);
    list_destroy (l);
    return (rc);
}

static char * shared_filename_create (spank_t sp)
{
    uint32_t jobid;
    uint32_t stepid;
    int      r;
    char buf [4096];

    if (spank_get_item (sp, S_JOB_ID, &jobid) != ESPANK_SUCCESS)
        slurm_error ("io-watchdog: Unable to get job id");

    if (spank_get_item (sp, S_JOB_STEPID, &stepid) != ESPANK_SUCCESS)
        slurm_error ("io-watchdog: Unable to get job stepid");

    srand (jobid);
    r = rand ();

    snprintf (buf, sizeof (buf), "/tmp/io-watchdog-%u.%u.%04x", 
             jobid, stepid, r);

    return (strdup (buf));
}

static int io_watchdog_opt_process (int val, const char *optarg, int remote)
{
    opts.enabled = 1;

    if (optarg == NULL)
        return (0);

    return (parse_watchdog_args (optarg));
}

static int do_setenv (spank_t sp, const char *name, const char *val)
{
    int rc = spank_setenv (sp, name, val, 1);
    if (rc != ESPANK_SUCCESS) {
        slurm_error ("Failed to set %s in task environment", name);
        return (-1);
    }
    return (0);
}

static int set_process_environment (spank_t sp)
{
    char buf [1024];
    int len = sizeof (buf);

    if (do_setenv (sp, "IO_WATCHDOG_SHARED_FILE", opts.shared_filename) < 0)
        return (-1);

    if (spank_getenv (sp, "LD_PRELOAD", buf, len) == ESPANK_SUCCESS) 
        strncat (buf, " io-watchdog-interposer.so", len - strlen (buf));
    else
        strncpy (buf, "io-watchdog-interposer.so", len);

    return (do_setenv (sp, "LD_PRELOAD", buf));
}

static int set_watchdog_environment (spank_t sp)
{
    if (opts.timeout) 
        do_setenv (sp, "IO_WATCHDOG_TIMEOUT", opts.timeout);

    if (opts.action)
        do_setenv (sp, "IO_WATCHDOG_ACTION", opts.action);

    if (opts.target)
        do_setenv (sp, "IO_WATCHDOG_TARGET", opts.target);

    if (opts.debug)
        do_setenv (sp, "IO_WATCHDOG_DEBUG", opts.debug);

    if (opts.argv0)
        do_setenv (sp, "IO_WATCHDOG_PROGRAM", opts.argv0);

    if (opts.conf)
        do_setenv (sp, "IO_WATCHDOG_CONFIG", opts.conf);

    if (opts.exact_timeout)
        do_setenv (sp, "IO_WATCHDOG_EXACT", opts.exact_timeout);

    return (0);
}

static int spawn_watchdog (spank_t sp)
{
    pid_t pid;
    char **env;

    char *args [] = { "io-watchdog", "--server", "-F", NULL, NULL };

    args [3] = opts.shared_filename;

    if (spank_get_item (sp, S_JOB_ENV, &env) != ESPANK_SUCCESS) {
        slurm_error ("io-watchdog: Failed to get process environment\n");
        return (-1);
    }

    if ((pid = fork ()) < 0)
        return (-1);
    if (pid > 0)
        return (0);


    if (execve (io_watchdog_server_path (), args, env) < 0)
        slurm_error ("io-watchdog: execve (%s): %m", args [0]);

    exit (1);
}

static int read_and_apply_config (spank_t sp, io_watchdog_conf_t conf)
{
    int argc;
    char **argv;

    if (io_watchdog_conf_parse_system (conf) < 0) {
        slurm_error ("io-watchdog: Unable to read default configuration\n");
        return (-1);
    }

    if (io_watchdog_conf_parse_user (conf, opts.conf) < 0) {
        slurm_error ("io-watchdog: Unable to read user configuration\n");
        return (-1);
    }

    if (spank_get_item (sp, S_JOB_ARGV, &argc, &argv) != ESPANK_SUCCESS) {
        slurm_error ("io-watchdog: Unable to get job argv");
        return (-1);
    }

    io_watchdog_conf_set_current_program (conf, (opts.argv0 = argv[0]));

    if (opts.rank < 0)
        opts.rank = io_watchdog_conf_rank (conf);

    return (0);
}

int slurm_spank_task_init (spank_t sp, int ac, char **av)
{
    int taskid;
    io_watchdog_conf_t conf;

    if (!opts.enabled)
        return (0);

    conf = io_watchdog_conf_create ();

    if (read_and_apply_config (sp, conf) < 0)
        return (-1);

    io_watchdog_conf_destroy (conf);

    if (spank_get_item (sp, S_TASK_GLOBAL_ID, &taskid) != ESPANK_SUCCESS) {
        slurm_error ("io-watchdog: unable to get task id");
        return (-1);
    }

    if (taskid != opts.rank) {
        slurm_info ("io-watchdog: this rank (%d) != %d. No action\n", 
                    taskid, opts.rank);
        return (0);
    }

    opts.shared_filename = shared_filename_create (sp);

    set_watchdog_environment (sp);

    spawn_watchdog (sp);

    set_process_environment (sp);

    return (0);
}

/*
 * vi: ts=4 sw=4 expandtab
 */

