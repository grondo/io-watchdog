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
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include <fcntl.h>

#include "conf.h"
#include "split.h"
#include "list.h"
#include "log_msg.h"
#include "shared.h"

/*****************************************************************************
 *  Options
 *****************************************************************************/

#define _GNU_SOURCE
#include <getopt.h>
struct option opt_table [] = {
    { "help",         0, NULL, 'h' },
    { "verbose",      0, NULL, 'v' },
    { "action",       1, NULL, 'a' },
    { "target",       1, NULL, 'T' },
    { "timeout",      1, NULL, 't' },
    { "rank",         1, NULL, 'r' },
    { "config",       1, NULL, 'f' },
    { "list-actions", 0, NULL, 'l' },
    { "server",       0, NULL, 'S' },
    { "shared-file",  1, NULL, 'F' },
    { "method",       1, NULL, 'm' },
    { NULL,           0, NULL, 0   }
};

const char * const opt_string = "hvlSm:a:t:T:r:F:f:";

#define USAGE "\
Usage: %s [OPTIONS] [executable args...]\n\
  -t, --timeout=N[smhd]  Set watchdog timeout to [N] seconds. Suffix may be\n\
                          s for seconds, m minutes, h hours, or d days. [N]\n\
                          may be an arbitrary floating-point number.\n\
  -a, --action=script,.. Run action [script] on watchdog trigger. More than\n\
                          one action may be specified.\n\
  -T, --target=glob      Only target processes with names matching the shell\n\
                          globbing pattern [glob] (See glob(7)). This is \n\
                          useful when running jobs under other processes like\n\
                          time(1), as it avoids enabling the io-watchdog on \n\
                          the time process.\n\
  -r, --rank=N           Only target rank [N] (default = 0) of a SLURM job.\n\
  -m, --method=TYPE      Specify the method used for the watchdog timeout.\n\
                          TYPE may be `sloppy' or `exact' (default = sloppy)\n\
  \n\
  -f, --config=file      Specify alternate config file [file]\n\
  \n\
  -l, --list-actions     Print list of pre-defined actions and exit.\n\
  \n\
  -v, --verbose          Increase io-watchdog verbosity.\n\
  -h, --help             Display this message.\n"


/*****************************************************************************
 *  Data Types
 *****************************************************************************/

struct io_watchdog_options {
    int                              argc;
    char **                          argv;
    char *                           timeout_string;
    char *                           env_action_string;
    char *                           shared_file; 
    char *                           target;
    char *                           config_file;

    double                           timeout;
    char *                           timeout_method;
    unsigned int                     exact_timeout;
    int                              verbose;
    int                              rank;
    int                              timeout_has_suffix;

    unsigned int                     server_only;
    char *                           target_program;             

    List                             actions;
};

struct prog_ctx {
    char *                           prog;
    io_watchdog_conf_t               conf;
    int                              pipe[2];
    struct io_watchdog_options       opts;
    struct io_watchdog_shared_info   *shared;
    struct io_watchdog_shared_region *shared_region;
};

/*****************************************************************************
 *  Prototypes
 *****************************************************************************/

static void prog_ctx_init (struct prog_ctx *ctx, int ac, char *av []);
static void prog_ctx_fini (struct prog_ctx *ctx);
static void process_env (struct prog_ctx *ctx);
static void parse_cmdline (struct prog_ctx *ctx, int ac, char *av []);
static void set_timeout_method (struct prog_ctx *ctx, const char *method);
static int  io_watchdog_server (struct prog_ctx *ctx);
static int  check_user_actions (struct prog_ctx *ctx);
static void list_all_actions (struct prog_ctx *ctx);
static void exec_user_args (struct prog_ctx *ctx);
static int  shared_region_create (struct prog_ctx *ctx);
static int  monitor_this_rank (struct prog_ctx *ctx);
static void set_process_environment (struct prog_ctx *ctx);
static int  wait_for_exec_completion (struct prog_ctx *ctx);
static void notify_watchdog_of_exec_failure (struct prog_ctx *ctx, int status);

/*****************************************************************************
 *  Functions
 *****************************************************************************/

int main (int ac, char *av[])
{
    int status = 1;
    pid_t pid;
    struct prog_ctx prog_ctx;

    prog_ctx_init (&prog_ctx, ac, av);
    process_env (&prog_ctx);
    parse_cmdline (&prog_ctx, ac, av);

    if (!monitor_this_rank (&prog_ctx))
        exec_user_args (&prog_ctx);
    else if ((status = shared_region_create (&prog_ctx)))
        log_err ("Failed to create shared file.\n");
    else if (prog_ctx.opts.server_only) 
        status = io_watchdog_server (&prog_ctx);
    else if ((pid = fork ()) < 0)
        log_err ("fork: %s\n", strerror (errno));
    else if (pid == 0)
        status = io_watchdog_server (&prog_ctx);
    else {
        set_process_environment (&prog_ctx);
        exec_user_args (&prog_ctx);
    }

    prog_ctx_fini (&prog_ctx);

    exit (status);
}

static int conf_init (struct prog_ctx *ctx)
{
    ctx->conf = io_watchdog_conf_create ();
    return (0);
}

static void conf_fini (struct prog_ctx *ctx)
{
    io_watchdog_conf_destroy (ctx->conf);
}

static int setenvi (const char *name, int val)
{
    char buf [64];
    snprintf (buf, 64, "%d", val);
    return (setenv (name, buf, 1));
}

static void set_process_environment (struct prog_ctx *ctx)
{
    char *val;

    if ((val = getenv ("LD_PRELOAD"))) {
        char buf [4096];
        snprintf (buf, sizeof (buf), "%s io-watchdog-interposer.so", val);
        setenv ("LD_PRELOAD", buf, 1);
    } else
        setenv ("LD_PRELOAD", "io-watchdog-interposer.so", 1);

    setenvi ("IO_WATCHDOG_DEBUG", ctx->opts.verbose);

    setenv  ("IO_WATCHDOG_SHARED_FILE", ctx->shared_region->path, 1);

    if (ctx->opts.target)
        setenv  ("IO_WATCHDOG_TARGET", ctx->opts.target, 1);

    if (ctx->opts.exact_timeout)
        setenv ("IO_WATCHDOG_EXACT", "1", 1);

}

static void exec_user_args (struct prog_ctx *ctx)
{
    log_debug ("execing process `%s'\n", ctx->opts.argv [0]);

    if (execvp (ctx->opts.argv[0], ctx->opts.argv) < 0) {
       log_err ("exec: %s: %s\n", ctx->opts.argv[0], strerror (errno));
       notify_watchdog_of_exec_failure (ctx, 127);
       exit (127);
    }
}

static void io_watchdog_options_init (struct io_watchdog_options *opts)
{
    memset (opts, 0, sizeof (*opts));
    opts->actions = list_create ((ListDelF) free);
}

static void set_close_on_exec (int fd)
{
    if (fcntl (fd, F_SETFD, FD_CLOEXEC) < 0)
        log_fatal (1, "failed to set close-on-exec for fd%d: %s\n",
                fd, strerror (errno));
}

static void prog_ctx_init (struct prog_ctx *ctx, int ac, char *av[])
{
    char *prog = basename (av [0]);

    memset (ctx, 0, sizeof (*ctx));

    ctx->prog = strdup (prog);

    log_msg_init (prog);

    io_watchdog_options_init (&ctx->opts);

    if (pipe (ctx->pipe) < 0)
        log_fatal (1, "failed to create exec status pipe: %s\n",
                strerror (errno));

    set_close_on_exec (ctx->pipe[0]);
    set_close_on_exec (ctx->pipe[1]);

    if (conf_init (ctx) < 0)
        log_fatal (1, "failed to initialize defuault configuration.\n");

}

static void io_watchdog_options_fini (struct io_watchdog_options *opts)
{
    if (opts->timeout_string)
        free (opts->timeout_string);
    if (opts->actions)
        list_destroy (opts->actions);
    memset (opts, 0, sizeof (*opts));
}

static void prog_ctx_fini (struct prog_ctx *ctx)
{
    if (ctx->prog)
        free (ctx->prog);

    if (ctx->shared_region)
        io_watchdog_shared_region_destroy (ctx->shared_region);

    ctx->shared = NULL;

    io_watchdog_options_fini (&ctx->opts);

    conf_fini (ctx);

    log_msg_fini ();
}

static void usage (const char *prog)
{
    fprintf (stderr, USAGE, prog);
}

static void xfree (void *x)
{
    if (x != NULL)
        free (x);
}

static int string_to_int (const char *str, int *p2int)
{
    char *p;
    long l;

    l = strtol (str, &p, 10);

    if ((*p != '\0') || (l < 0))
        return (-1);

    *p2int = (int) l;

    return (0);
}

static int monitor_this_rank (struct prog_ctx *ctx)
{
    char *val;
    int rank;

    if (!(val = getenv ("SLURM_PROCID")))
        return (1);
    
    string_to_int (val, &rank);

    if (rank == ctx->opts.rank)
        return (1);

    return (0);
}


static void process_env (struct prog_ctx *ctx)
{
    char *val;

    if ((val = getenv ("IO_WATCHDOG_DEBUG"))) {
        int level = atoi (val);
        if (level > 0)
            ctx->opts.verbose = level;
    }

    if ((val = getenv ("IO_WATCHDOG_TIMEOUT"))) {
        double d;
        int    has_suffix;

        if (parse_timeout_string (val, &d, &has_suffix) < 0)
            log_fatal (1, "IO_WATCHDOG_TIMEOUT=%s Invalid. Ignoring\n", val);
        else {
            ctx->opts.timeout_string = strdup (val);
            ctx->opts.timeout = d;
            ctx->opts.timeout_has_suffix = has_suffix;
        }
    }

    if ((val = getenv ("IO_WATCHDOG_EXACT")) && val[0] != '0')
        ctx->opts.exact_timeout = 1;

    if ((val = getenv ("IO_WATCHDOG_ACTION")))
        ctx->opts.env_action_string = strdup (val);

    if ((val = getenv ("IO_WATCHDOG_TARGET")))
        ctx->opts.target = strdup (val);

    if ((val = getenv ("IO_WATCHDOG_SHARED_FILE")))
        ctx->opts.shared_file = strdup (val);

    if ((val = getenv ("IO_WATCHDOG_PROGRAM")))
        ctx->opts.target_program = strdup (val);

    if (  (val = getenv ("IO_WATCHDOG_RANK")) 
       && (string_to_int (val, &ctx->opts.rank) < 0))
        log_err ("IO_WATCHDOG_RANK=%s invalid. Ignoring.\n", val);

    if ((val = getenv ("IO_WATCHDOG_CONFIG")))
        ctx->opts.config_file = strdup (val);
}

static char * xstrdup (const char *str)
{
    if (str == NULL)
        return (NULL);
    return (strdup (str));
}

static void apply_config (struct prog_ctx *ctx)
{

    if (ctx->opts.server_only) {
        if (ctx->opts.target_program)
            io_watchdog_conf_set_current_program (ctx->conf, 
                                                  ctx->opts.target_program);
        else
            log_fatal (1, "Must set target program with --server\n");
    } else
        io_watchdog_conf_set_current_program (ctx->conf, ctx->opts.argv[0]);

    if (!ctx->opts.timeout_string) {
        const char *str = io_watchdog_conf_timeout_string (ctx->conf);
        int has_suffix = io_watchdog_conf_timeout_has_suffix (ctx->conf);

        ctx->opts.timeout_string = xstrdup (str);
        ctx->opts.timeout_has_suffix = has_suffix;
        ctx->opts.timeout = io_watchdog_conf_timeout (ctx->conf);
    }

    if (list_count (ctx->opts.actions) == 0) {
        list_destroy (ctx->opts.actions);
        ctx->opts.actions = io_watchdog_conf_actions (ctx->conf);
    }

    if (!ctx->opts.target)
        ctx->opts.target = xstrdup (io_watchdog_conf_target (ctx->conf));

    if (!ctx->opts.timeout_method 
       && io_watchdog_conf_exact_timeout (ctx->conf)) {
        ctx->opts.exact_timeout = 1;
    }
}

static void parse_cmdline (struct prog_ctx *ctx, int ac, char *av[])
{
    int list_actions = 0;

    setenv ("POSIXLY_CORRECT", "1", 1);
    for (;;) {
        char c = getopt_long (ac, av, opt_string, opt_table, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage (ctx->prog);
            exit (0);
        case 'v':
            ctx->opts.verbose++;
            break;
        case 't':
            xfree (ctx->opts.timeout_string);
            ctx->opts.timeout_string = strdup (optarg);
            if (parse_timeout_string (optarg, 
                                      &ctx->opts.timeout, 
                                      &ctx->opts.timeout_has_suffix) < 0)
                log_fatal (1, "Invalid timeout string `%s'\n", optarg);
            break;
        case 'm':
            ctx->opts.timeout_method = strdup (optarg);
            break;
        case 'a':
            ctx->opts.actions = 
                list_split_append (ctx->opts.actions, ":,", optarg);
            break;
        case 'T':
            xfree (ctx->opts.target);
            ctx->opts.target = strdup (optarg);
            break;
        case 'r':
            if (string_to_int (optarg, &ctx->opts.rank) < 0)
                log_fatal (1, "Invalid rank `%s'\n", optarg);
            break;
        case 'l':
            list_actions = 1;
            break;
        case 'S':
            ctx->opts.server_only = 1;
            break;
        case 'F':
            xfree (ctx->opts.shared_file);
            ctx->opts.shared_file = strdup (optarg);
            break;
        case 'f':
            xfree (ctx->opts.config_file);
            ctx->opts.config_file = strdup (optarg);
            break;
        case '?':
            if (optopt > 0)
                log_err ("Invalid option \"-%c\"\n", optopt);
            else
                log_err ("Invalid option \"%s\"\n", av [optind - 1]);
            break;
        default:
            log_err ("Unimplemented option \"%s\"\n", av [optind - 1]);
            break;
        }
    }


    if (ctx->opts.verbose)
        log_msg_set_verbose (ctx->opts.verbose);

    if (io_watchdog_conf_parse_system (ctx->conf) < 0)
        log_fatal (1, "Failed to read default system configuration\n");

    if (io_watchdog_conf_parse_user (ctx->conf, ctx->opts.config_file) < 0)
        log_fatal (1, "Failed to read user configuration\n");

    if (list_actions) {
        list_all_actions (ctx);
        exit (0);
    }

    if (!ctx->opts.server_only) {
        if ((ctx->opts.argc = ac - optind) == 0)
            log_fatal (1, "Must supply executable to run under io-watchdog.\n");
        ctx->opts.argv = &av [optind];
    }

    if (!list_count (ctx->opts.actions) && ctx->opts.env_action_string)
        ctx->opts.actions = list_split_append (ctx->opts.actions, ",:", 
                                               ctx->opts.env_action_string);

    set_timeout_method (ctx, ctx->opts.timeout_method);

    apply_config (ctx);

    if (ctx->opts.server_only && av [optind])
        log_fatal (1, "Do not specify any program to run with --server\n");

    if (ctx->opts.server_only && !ctx->opts.shared_file) 
        log_fatal (1, "Must supply shared file name in server-only mode.\n");

    if (check_user_actions (ctx) < 0)
        log_fatal (1, "Invalid action\n");

    return;
}

static void list_all_actions (struct prog_ctx *ctx)
{
    log_verbose ("list all actions\n");
    io_watchdog_conf_list_actions (ctx->conf);
}

static int check_action (char *name, struct prog_ctx *ctx)
{
    /*
     *  Check for system || user defined action, or explicit path
     *   to script
     */
    if (  io_watchdog_conf_find_action (ctx->conf, name) 
       || (access (name, R_OK | X_OK) == 0) )
        return (0);

    log_err ("Invalid action \"%s\": %s\n", name, strerror (errno));
    return (-1);
}

static int check_user_actions (struct prog_ctx *ctx)
{
    if (!ctx->opts.actions)
        return (0);
    return (list_for_each (ctx->opts.actions, (ListForF) check_action, ctx));
}

static void set_timeout_method (struct prog_ctx *ctx, const char *method)
{
    if (!method)
        return;

    if (strcmp (method, "sloppy") == 0)
        return;

    if (strcmp (method, "exact") == 0) {
        ctx->opts.exact_timeout = 1;
        return;
    }   

    log_fatal (1, "Invalid timeout method \"%s\" specified.\n", method);
}


/******************************************************************************
 *  Forked io_watchdog server 
 ******************************************************************************/

/* 
   Subtract the `struct timespec' values X and Y by computing X - Y.
   If the difference is negative or zero, return 0.
   Otherwise, return 1 and store the difference in DIFF.
   X and Y must have valid ts_nsec values, in the range 0 to 999999999.
   If the difference would overflow, store the maximum possible difference.  */

#define TIME_T_MAX ((time_t) INT_MAX)

static int timespec_subtract (struct timespec *diff,
                              struct timespec const *x, 
                              struct timespec const *y)
{
    time_t sec = x->tv_sec - y->tv_sec;
    long int nsec = x->tv_nsec - y->tv_nsec;

    if (x->tv_sec < y->tv_sec)
        return 0;

    if (sec < 0)
    {
        /* The difference has overflowed.  */
        sec =  999999999;
        nsec = 999999999;
    }
    else if (sec == 0 && nsec <= 0)
        return 0;

    if (nsec < 0)
    {
        sec--;
        nsec += 1000000000;
    }

    diff->tv_sec = sec;
    diff->tv_nsec = nsec;
    return 1;
}

static int gettime (struct timespec *ts)
{
    struct timeval tv;
    int r;

    if ((r = gettimeofday (&tv, 0)) == 0)  {
        ts->tv_sec = tv.tv_sec;
        ts->tv_nsec = tv.tv_usec * 1000;
    }

    return (r);
}

static void update_timeout_env (struct prog_ctx *ctx)
{
    char buf [64];

    snprintf (buf, 64, "%.3f", ctx->opts.timeout);
    if (setenv ("IO_WATCHDOG_TIMEOUT", buf, 1) < 0)
        log_err ("Failed to set IO_WATCHDOG_TIMEOUT=%s: %s\n",
                buf, strerror (errno));
}

static void update_timeout (struct prog_ctx *ctx, double timeout)
{
    char buf [1024];

    ctx->opts.timeout = timeout;

    /*
     *  Update timeout string:
     */
    if (ctx->opts.timeout_string)
        free (ctx->opts.timeout_string);
    snprintf (buf, sizeof (buf), "%.3fs", ctx->opts.timeout);
    ctx->opts.timeout_string = strdup (buf);

    /*
     *  Update environment for action scripts
     */
    update_timeout_env (ctx);
}

static int client_timeout_wait (struct prog_ctx *ctx, struct timespec *ts)
{
    struct io_watchdog_shared_info *s = ctx->shared;

    /*
     *  First, check to make sure condition (a pending client request)
     *   isn't already true, if not, then sleep until next timeout or
     *   client request.
     */
    pthread_mutex_lock (&s->mutex);
    if (s->req_type == IO_REQ_NONE) {
        int rc = pthread_cond_timedwait (&s->cond, &s->mutex, ts);
        /*
         *  If we timed out or got an error return to caller immediately.
         */
        if (rc != 0) {
            pthread_mutex_unlock (&s->mutex);
            return (rc == ETIMEDOUT ? 0 : -1);
        }
    }

    /*
     *  Otherwise, we were signalled by the client and have to
     *   process a request
     */
    log_debug ("Handling request %d\n", s->req_type);
    switch (s->req_type) {
        case IO_REQ_NONE:
            break;
        case IO_REQ_GET_TIMEOUT:
            s->info.timeout = ctx->opts.timeout;
            break;
        case IO_REQ_SET_TIMEOUT:
            update_timeout (ctx, s->info.timeout);
            break;
    }

    /*
     * Once the request is processed, reset req_type in the server
     *  and wait for client at the barrier.
     */
    s->req_type = IO_REQ_NONE;
    pthread_mutex_unlock (&s->mutex);

    io_watchdog_shared_info_barrier (s);

    return (1);
}


/* Taken from coreutils/lib:
   
   Sleep until the time (call it WAKE_UP_TIME) specified as
   SECONDS seconds after the time this function is called.
   SECONDS must be non-negative.  If SECONDS is so large that
   it is not representable as a `struct timespec', then use
   the maximum value for that interval.  Return -1 on failure
   (setting errno), 0 on success.  */

static int xnanowait (struct prog_ctx *ctx, double seconds)
{
    int overflow;
    int rc;
    double ns;
    struct timespec ts_start;
    struct timespec ts_sleep;
    struct timespec ts_stop;

    assert (0 <= seconds);

    if (gettime (&ts_start) != 0)
        return -1;

    /* Separate whole seconds from nanoseconds.
       Be careful to detect any overflow.  */
    ts_sleep.tv_sec = seconds;
    ns = 1e9 * (seconds - ts_sleep.tv_sec);
    overflow = ! (ts_sleep.tv_sec <= seconds && 0 <= ns && ns <= 1e9);
    ts_sleep.tv_nsec = ns;

    /* Round up to the next whole number, if necessary, so that we
       always sleep for at least the requested amount of time.  Assuming
       the default rounding mode, we don't have to worry about the
       rounding error when computing 'ns' above, since the error won't
       cause 'ns' to drop below an integer boundary.  */
    ts_sleep.tv_nsec += (ts_sleep.tv_nsec < ns);

    /* Normalize the interval length.  nanosleep requires this.  */
    if (1000000000 <= ts_sleep.tv_nsec)
    {
        time_t t = ts_sleep.tv_sec + 1;

        /* Detect integer overflow.  */
        overflow |= (t < ts_sleep.tv_sec);

        ts_sleep.tv_sec = t;
        ts_sleep.tv_nsec -= 1000000000;
    }

    /* Compute the time until which we should sleep.  */
    ts_stop.tv_sec = ts_start.tv_sec + ts_sleep.tv_sec;
    ts_stop.tv_nsec = ts_start.tv_nsec + ts_sleep.tv_nsec;
    if (1000000000 <= ts_stop.tv_nsec)
    {
        ++ts_stop.tv_sec;
        ts_stop.tv_nsec -= 1000000000;
    }

    /* Detect integer overflow.  */
    overflow |= (ts_stop.tv_sec < ts_start.tv_sec
             || (ts_stop.tv_sec == ts_start.tv_sec
             && ts_stop.tv_nsec < ts_start.tv_nsec));

    if (overflow)
    {
        /* Fix ts_sleep and ts_stop, which may be garbage due to overflow.  */
        ts_sleep.tv_sec = ts_stop.tv_sec = TIME_T_MAX;
        ts_sleep.tv_nsec = ts_stop.tv_nsec = 999999999;
    }

    while ((rc = client_timeout_wait (ctx, &ts_stop)) != 0)
    {
        if (rc < 0)
            return -1;
    }

    return 0;
}

static void setup_server_environment (struct prog_ctx *ctx)
{
    char buf [64];

    snprintf (buf, 16, "%ld", (long) ctx->shared->monitored_pid);
    if (setenv ("IO_WATCHDOG_PID", buf, 1) < 0)
        log_err ("Failed to set IO_WATCHDOG_PID=%s: %s\n", 
                buf, strerror (errno));
    /*
     *  Remove unneeded env vars
     */
    unsetenv ("IO_WATCHDOG_DEBUG");
    unsetenv ("IO_WATCHDOG_TARGET");
    unsetenv ("IO_WATCHDOG_TIMEOUT");
    unsetenv ("IO_WATCHDOG_ACTION");
    unsetenv ("IO_WATCHDOG_CONFIG");
    unsetenv ("IO_WATCHDOG_SHARED_FILE");

    /*
     *  Set environment variables for actions
     */
    update_timeout_env (ctx);

    if (setenv ("IO_WATCHDOG_TARGET", ctx->shared->cmd, 1) < 0)
        log_err ("Failed to set IO_WATCHDOG_TARGET=%s: %s\n",
                ctx->shared->cmd, strerror (errno));

    /*
     * XXX: Add support for just removing ourselves from LD_PRELOAD
     */
    unsetenv ("LD_PRELOAD");

    return;
}

static int run_action (char *name, struct prog_ctx *ctx)
{
    int status = 0;
    const char *path = io_watchdog_conf_find_action (ctx->conf, name);

    if (path) {
        log_msg ("Running pre-defined action [%s = \"%s\"]\n", name, path);
        status = system (path);
    } else {
        log_msg ("Running [%s]\n", name);
        status = system (name);
    }

    if (status)
        log_err ("%s: exited with status 0x%04x\n", name, status);

    return (0);

}

static char * timeout_units (struct prog_ctx *ctx)
{
    return (ctx->opts.timeout_has_suffix ? "" : "s");
}

static int invoke_watchdog_action (struct prog_ctx *ctx)
{
    log_err ("%s: Nothing written for %s%s. Timing out\n",
            ctx->shared->cmd, ctx->opts.timeout_string, timeout_units (ctx));

    log_verbose ("Invoking %d actions\n", list_count (ctx->opts.actions));
    list_for_each (ctx->opts.actions, (ListForF) run_action, ctx);

    return (0);
}

static void wait_for_application (struct prog_ctx *ctx)
{
    log_debug2 ("waiting for application...\n");
    io_watchdog_shared_info_barrier (ctx->shared);
    log_debug2 ("synchronization complete.\n");
}

static double get_next_timeout (struct prog_ctx *ctx)
{
    struct timespec ts, lastio, result;
    double ago;

    if (!timerisset (&ctx->shared->lastio) || !ctx->shared->flag)
        return (ctx->opts.timeout);

    if (gettime (&ts) < 0) {
        log_err ("gettimeofday: %s\n", strerror (errno));
        return (ctx->opts.timeout);
    }

    lastio.tv_sec = ctx->shared->lastio.tv_sec;
    lastio.tv_nsec = ctx->shared->lastio.tv_usec * 1000;

    if (!timespec_subtract (&result, &ts, &lastio))
        return (0.0);

    if ((ago = result.tv_sec + (result.tv_nsec / 1e9)) > ctx->opts.timeout)
        return (0.0);

    return (ctx->opts.timeout - ago);
}

static int io_watchdog_server (struct prog_ctx *ctx)
{
    int warned = 0;

    ctx->shared->server_pid = getpid ();

    if (wait_for_exec_completion (ctx) < 0)
        log_fatal (1, "Unable to exec process. Server exiting\n");

    wait_for_application (ctx);
    setup_server_environment (ctx);

    log_verbose ("server process (%ld) started with timeout = (%s%s) %.3fs\n", 
                 getpid (), ctx->opts.timeout_string, timeout_units (ctx),
                 ctx->opts.timeout);

    if (ctx->shared->exited) {
        log_verbose ("server: monitored process exited\n");
        exit (0);
    }

    for (;;) {
        double timeout = get_next_timeout (ctx);

        ctx->shared->flag = 0;
        log_debug2 ("Next timeout in %.3fs\n", timeout);
        xnanowait (ctx, timeout);

        if (ctx->shared->exited) {
            log_debug2 ("server: monitored process exited\n");
            return (0);
        }

        log_debug2 ("server: wakeup: flag = %d\n", ctx->shared->flag);

        if (!ctx->shared->flag && !warned++) {
           invoke_watchdog_action (ctx);
        }

    } 

    exit (0);
}

static int shared_region_create (struct prog_ctx *ctx)
{
    char *f = ctx->opts.shared_file;

    if (!(ctx->shared_region = io_watchdog_shared_region_create (f))) {
        log_err ("Failed to create shared memory region.\n");
        return (1);
    }

    ctx->shared = ctx->shared_region->shared;
    return (0);
}

static void notify_watchdog_of_exec_failure (struct prog_ctx *ctx, int status)
{
    write (ctx->pipe[1], (char *) &status, 1);
}

static int wait_for_exec_completion (struct prog_ctx *ctx)
{
    int rc;
    char c;

    close (ctx->pipe[1]);

    while (((rc = read (ctx->pipe[0], &c, 1)) < 0) && (errno == EINTR)) {}

    close (ctx->pipe[0]);

    if (rc == 0) /*  exec() completed normally */
        return (0);


    return (-1);
}

/*
 * vi: ts=4 sw=4 expandtab
 */
