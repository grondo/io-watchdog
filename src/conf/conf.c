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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <fnmatch.h>
#include <dirent.h>
#include <errno.h>

#include "split.h"
#include "list.h"
#include "log_msg.h"
#include "conf.h"

/*****************************************************************************
 *  Constants
 *****************************************************************************/

static const char * default_config =      "/etc/io-watchdog/io-watchdog.conf";
static const char * default_action_path = "/etc/io-watchdog/scripts"
                                          ":/usr/share/io-watchdog/scripts";
static const char * default_timeout =     "1.0h";
static const int    default_rank =        0;

/*****************************************************************************
 *  Data Types
 *****************************************************************************/

struct io_watchdog_action_entry {
    char * name;
    char * path;
};

struct io_watchdog_timeout {
    double val;
    char * str;
    int    has_suffix;
};

struct io_watchdog_conf_entry {
    char * program;
    struct io_watchdog_timeout timeout;
    char * target;
    int    rank;
    int    exact_timeout;
    List   actions;
};

struct io_watchdog_conf {
    struct io_watchdog_conf_entry *default_entry;
    struct io_watchdog_conf_entry *current;
    int  user_conf;
    List entries;
    List defined_actions; /* actions defined explicitly in conf file */
    List system_actions;  /* actions found in system path            */
    List system_paths;    /* paths for system actions                */
    List user_paths;      /* Search path for user actions            */
};

/*****************************************************************************
 *  Functions
 *****************************************************************************/

static void xfree (void *x)
{
    if (x) free (x);
}

static struct io_watchdog_action_entry *
io_watchdog_action_entry_create (char *name, char *path)
{
    struct io_watchdog_action_entry *a = malloc (sizeof (*a));

    if (a == NULL)
        return (NULL);

    a->name = strdup (name);
    a->path = strdup (path); 
    return (a);
}

static void 
io_watchdog_action_entry_destroy (struct io_watchdog_action_entry *a) 
{
    if (a != NULL) {
        xfree (a->name);
        xfree (a->path);
        free (a);
    }
}

static struct io_watchdog_conf_entry * 
io_watchdog_conf_entry_create (char *program)
{
    struct io_watchdog_conf_entry *e = malloc (sizeof (*e));

    if (e == NULL)
        return (NULL);

    memset (e, 0, sizeof (*e));

    e->program = strdup (program);
    e->actions = list_create ((ListDelF) free);

    return (e);
}

static void io_watchdog_conf_entry_destroy (struct io_watchdog_conf_entry *e)
{
    if (e != NULL) {
        xfree (e->program);
        xfree (e->timeout.str);
        xfree (e->target);
        list_destroy (e->actions);
        free (e);
    }
}

static void create_default_entry (io_watchdog_conf_t conf)
{
    struct io_watchdog_conf_entry * e;

    io_watchdog_conf_new_program (conf, "default");
    conf->default_entry = conf->current = list_peek (conf->entries);

    e = conf->default_entry;

    e->rank = default_rank;
    e->timeout.str = strdup (default_timeout);
    parse_timeout_string (default_timeout, &e->timeout.val, 
                          &e->timeout.has_suffix);
}

static int parse_if_exists (io_watchdog_conf_t conf, const char *file)
{
    if (access (file, F_OK) < 0)
        return (0);

    if (access (file, R_OK) < 0) {
        log_err ("File %s exists but is not readable.\n", file);
        return (-1);
    }

    if (io_watchdog_conf_parse (conf, file) < 0) 
        return (-1);

    /* Successfully read config file */
    return (0);
}

static char * user_conf_file (char *buf, int len)
{
    const char *home;

    if (!(home = getenv ("HOME")))
        return (NULL);

    snprintf (buf, len, "%s/.io-watchdogrc", home);
    return (buf);
}

static int append_actions (char *path, io_watchdog_conf_t conf)
{
    DIR *dir = opendir (path);
    struct dirent *d;

    if (dir == NULL) {
        if (errno == ENOENT)
            return (0);
        log_err ("opendir (%s): %s\n", path, strerror (errno));
        return (-1);
    }

    while ((d = readdir (dir))) {
        char buf [4096];

        if (d->d_name[0] == '.')
            continue;

        snprintf (buf, sizeof (buf), "%s/%s", path, d->d_name);

        if (access (buf, X_OK) == 0) {
            struct io_watchdog_action_entry *a;
            if ((a = io_watchdog_action_entry_create (d->d_name, buf)))
                list_append (conf->system_actions, a);
            else
                log_err ("Failed to create action for %s\n", buf);
        }
    }
    closedir (dir);
    return (0);
}

static int find_system_actions (io_watchdog_conf_t conf)
{
    return (list_for_each (conf->system_paths, (ListForF) append_actions, conf));
}

static int action_print (struct io_watchdog_action_entry *a, void *arg)
{
    printf ("%-25s %-50s\n", a->name, a->path);
    return (0);
}

int io_watchdog_conf_list_actions (io_watchdog_conf_t conf)
{
    struct io_watchdog_action_entry hdr = { "NAME", "PATH" };
    if (list_count (conf->system_actions)) {
        log_msg ("%d System actions:\n", list_count (conf->system_actions));
        action_print (&hdr, NULL);
        list_for_each (conf->system_actions, (ListForF) action_print, NULL);
    } else
        log_msg ("No system defined actions.\n");

    if (list_count (conf->defined_actions)) {
        log_msg ("%d User defined actions:\n", 
                list_count (conf->defined_actions));
        action_print (&hdr, NULL);
        list_for_each (conf->defined_actions, (ListForF) action_print, NULL);
    } else
        log_msg ("No user defined actions.\n");

    return (0);
}

static int find_action (struct io_watchdog_action_entry *a, char *name)
{
    return (strcmp (a->name, name) == 0);
}

static const char * user_path_search (io_watchdog_conf_t conf, char *name)
{
    ListIterator i;
    char *dir = NULL;
    const char *path = NULL;

    i = list_iterator_create (conf->user_paths);
    while ((dir = list_next (i))) {
        char buf [4096];
        snprintf (buf, sizeof (buf), "%s/%s", dir, name);
        if (access (buf, X_OK) == 0) {
            struct io_watchdog_action_entry *a;
            a = io_watchdog_action_entry_create (name, buf);
            list_append (conf->defined_actions, a);
            path = a->path;
            break;
        }
    }
    list_iterator_destroy (i);

    return (path);
}

const char * io_watchdog_conf_find_action (io_watchdog_conf_t conf, char *name)
{
    struct io_watchdog_action_entry *a;
    const char *path = NULL;

    if (name == NULL)
        return (NULL);

    if ((a = list_find_first (conf->defined_actions, 
                              (ListFindF) find_action, name)))
        return (a->path);

    if ((path = user_path_search (conf, name)))
        return (path);

    if ((a = list_find_first (conf->system_actions, 
                              (ListFindF) find_action, name)))
        return (a->path);

    return (NULL);
}

int io_watchdog_conf_parse_system (io_watchdog_conf_t conf)
{
    conf->user_conf = 0;

    if (parse_if_exists (conf, default_config) < 0)
        return (-1);

    if (find_system_actions (conf) < 0)
        return (-1);

    return (0);
}

int io_watchdog_conf_parse_user (io_watchdog_conf_t conf, const char *path)
{
    char buf [1024];

    if (!path && !(path = user_conf_file (buf, 1024)))
        return (0);

    conf->user_conf = 1;

    return (parse_if_exists (conf, path));
}

int io_watchdog_conf_parse_defaults (io_watchdog_conf_t conf)
{
    if (io_watchdog_conf_parse_system (conf) < 0)
        return (-1);

    conf->user_conf = 1;

    if (io_watchdog_conf_parse_user (conf, NULL))
        return (-1);

    return (0);
}

io_watchdog_conf_t io_watchdog_conf_create ()
{
    char *cp;

    io_watchdog_conf_t conf = malloc (sizeof (*conf));

    if (conf == NULL)
        return (NULL);

    conf->defined_actions = 
        list_create ((ListDelF) io_watchdog_action_entry_destroy);

    conf->system_actions = 
        list_create ((ListDelF) io_watchdog_action_entry_destroy);

    conf->entries = list_create ((ListDelF) io_watchdog_conf_entry_destroy);

    conf->system_paths = list_split (":", (cp = strdup (default_action_path)));
    free (cp);

    conf->user_paths = list_create ((ListDelF) free);

    create_default_entry (conf);

    return (conf);
}

void io_watchdog_conf_destroy (io_watchdog_conf_t conf)
{
    if (conf == NULL)
        return;

    list_destroy (conf->defined_actions);
    list_destroy (conf->system_actions);
    list_destroy (conf->entries);
    list_destroy (conf->system_paths);
    list_destroy (conf->user_paths);

    free (conf);
}


int 
io_watchdog_conf_append_action (io_watchdog_conf_t conf, char *name, char *path)
{
    struct io_watchdog_action_entry *a = NULL;

    if (!conf || !conf->defined_actions)
        return (-1);

    if (!(a = io_watchdog_action_entry_create (name, path)))
        return (-1);

    log_debug2 ("append action %s:%s\n", name, path);
    list_prepend (conf->defined_actions, a);

    return (0);
}

int io_watchdog_conf_new_program (io_watchdog_conf_t conf, char *prog)
{
    struct io_watchdog_conf_entry *e;

    if (!conf || !conf->entries)
        return (-1);

    if (!(e = io_watchdog_conf_entry_create (prog))) 
        return (-1);

    list_prepend (conf->entries, e);
    conf->current = e;

    return (0);
}

int io_watchdog_conf_set_timeout (io_watchdog_conf_t conf, char *timeout)
{
    int suffix;
    double d;
    if (!conf || !conf->current)
        return (-1);

    xfree (conf->current->timeout.str);
    conf->current->timeout.str = strdup (timeout);

    if (parse_timeout_string (timeout, &d, &suffix) < 0) {
        return (-1);
    }

    conf->current->timeout.val = d;
    conf->current->timeout.has_suffix = suffix;

    return (0);
}

int io_watchdog_conf_set_exact_timeout (io_watchdog_conf_t conf, int exact)
{
    if (!conf || !conf->current)
        return (-1);

    conf->current->exact_timeout = exact;
    return 0;
}

int io_watchdog_conf_set_actions (io_watchdog_conf_t conf, char *actions)
{
    List l;

    if (!conf || !conf->current)
        return (-1);

    l = conf->current->actions;
    conf->current->actions = list_split_append (l, ",:", actions); 
    return (0);
}

int io_watchdog_conf_set_target (io_watchdog_conf_t conf, char *target)
{
    if (!conf || !conf->current)
        return (-1);

    xfree (conf->current->target);
    conf->current->target = strdup (target);

    return (0);
}

int io_watchdog_conf_set_rank (io_watchdog_conf_t conf, int rank)
{
    if (!conf || !conf->current)
        return (-1);

    conf->current->rank = rank;
    return (0);
}

int io_watchdog_conf_append_action_path (io_watchdog_conf_t conf, char *path)
{
    char *p;

    if (!conf || !conf->system_paths || !conf->user_paths)
        return (-1);

    if (!(p = strdup (path)))
        return (-1);

    if (conf->user_conf)
        list_append (conf->user_paths, p);
    else
        list_append (conf->system_paths, p);

    return (0);
}

static int find_match (struct io_watchdog_conf_entry *e, char *prog)
{
    int match = 0;
    char *fn = strdup (prog);
    char *bn = basename (fn);

    if (fnmatch (e->program, prog, 0) == 0)
        match = 1;
    else if (fnmatch (e->program, bn, 0) == 0)
        match = 1;

    free (fn);

    return (match);
}

int io_watchdog_conf_set_current_program (io_watchdog_conf_t conf, char *prog)
{
    struct io_watchdog_conf_entry *e;

    if (!prog || !conf || !conf->entries)
        return (-1);

    if (!(e = list_find_first (conf->entries, (ListFindF) find_match, prog)))
        e = conf->default_entry;

    log_debug ("setting current config entry for %s to %s\n", prog, e->program);

    conf->current = e;

    return (0);
}

const char * io_watchdog_conf_current_program (io_watchdog_conf_t conf)
{
    return (conf->current->program);
}

double io_watchdog_conf_timeout (io_watchdog_conf_t conf)
{
    if (conf->current->timeout.str)
        return (conf->current->timeout.val);
    return (conf->default_entry->timeout.val);
}

const char * io_watchdog_conf_timeout_string (io_watchdog_conf_t conf)
{
    if (conf->current->timeout.str)
        return (conf->current->timeout.str);
    return (conf->default_entry->timeout.str);
}

int io_watchdog_conf_timeout_has_suffix (io_watchdog_conf_t conf)
{
    if (conf->current->timeout.str)
        return (conf->current->timeout.has_suffix);
    return (conf->default_entry->timeout.has_suffix);
}

int io_watchdog_conf_exact_timeout (io_watchdog_conf_t conf)
{
    if (conf->current)
        return (conf->current->exact_timeout);
    return (conf->default_entry->exact_timeout);
}

List io_watchdog_conf_actions (io_watchdog_conf_t conf)
{
    if (conf->current->actions)
        return (conf->current->actions);
    return (conf->default_entry->actions);
}

int io_watchdog_conf_rank (io_watchdog_conf_t conf)
{
    if (conf->current->rank >= 0)
        return (conf->current->rank);
    return (conf->default_entry->rank);
}

const char * io_watchdog_conf_target (io_watchdog_conf_t conf)
{
    char *target = conf->current->target;

    if (!target && (conf->current != conf->default_entry))
        target = conf->current->program;

    return (target);
}

extern const char *io_watchdog_path;

const char * io_watchdog_server_path ()
{
    return (io_watchdog_path);
}


static int apply_suffix (double *x, char suffix_char)
{
    unsigned int multiplier;

    switch (suffix_char)
    {
        case 0:
        case 's':
            multiplier = 1;
            break;
        case 'm':
            multiplier = 60;
            break;
        case 'h':
            multiplier = 60 * 60;
            break;
        case 'd':
            multiplier = 60 * 60 * 24;
            break;
        default:
            multiplier = 0;
    }

    if (multiplier == 0)
        return 1;

    *x *= multiplier;

    return 0;
}


int parse_timeout_string (const char *s, double *dp, int *has_suffix)
{
    double d;
    char *p;

    d = strtod (s, &p);

    if (! (0 <= d) /* Non-negative. */
       /* No chars after optional s,m,h.d */
       || (*p && *(p+1))
       /* Check for valid suffix char and apply */
       || apply_suffix (&d, *p))
    {
        /* log_err ("Invalid time interval `%s'\n", s); */
        return (-1);
    }

    log_debug2 ("User supplied timeout = (%s) %.3fs\n", s, d);

    *dp = d;
    *has_suffix = (*p != '\0');

    return (0);
}


/*
 * vi: ts=4 sw=4 expandtab
 */
