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
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>
#include <fnmatch.h>
#include <limits.h>
#define __USE_GNU /* program_invocation_short_name */
#include <errno.h>
#undef  __USE_GNU

#include <dlfcn.h>
#include <glob.h>
#include <time.h>

#include "log_msg.h"
#include "shared.h"

/******************************************************************************
 *  Typedefs of replaced functions
 ******************************************************************************/

typedef ssize_t (*write_f) (int fd, const void *buf, size_t count);
typedef ssize_t (*writev_f) (int fd, const struct iovec *v, int count);
typedef size_t  (*pwrite_f) (int fd, const void *buf, size_t s, off_t off);
typedef size_t  (*fwrite_f) (const void *ptr, size_t size, size_t n, FILE *fp);
typedef size_t  (*fwrite_unlocked_f) (const void *ptr, size_t size, 
                                      size_t n, FILE *stream);

typedef int     (*vfprintf_f) (FILE *fp, const char *fmt, va_list args);
typedef int     (*vprintf_f)  (const char *fmt, va_list args);

typedef int     (*fputc_f) (int c, FILE *fp);
typedef int     (*fputs_f) (const char *s, FILE *fp);
typedef int     (*putc_f)  (int c, FILE *fp);
typedef int     (*putchar_f) (int c);
typedef int     (*puts_f)  (const char *str);
typedef int     (*fputs_unlocked_f) (const char *s, FILE *fp);

/******************************************************************************
 *  Data Types
 ******************************************************************************/

struct io_watchdog_ops {
    vfprintf_f        vfprintf;
    vprintf_f         vprintf;
    fputc_f           fputc;
    fputs_f           fputs;
    putc_f            putc;
    putchar_f         putchar;
    puts_f            puts;
    write_f           write;
    writev_f          writev;
    pwrite_f          pwrite;
    fwrite_f          fwrite;
    fwrite_unlocked_f fwrite_unlocked;
    fputs_unlocked_f  fputs_unlocked;
    putc_f            _IO_putc;
    puts_f            _IO_puts;
};

struct io_watchdog_sym_info {
    const char *name;
    void **symp;
};

struct io_watchdog_ctx {
    void *libc;

    struct io_watchdog_shared_region *shared_region;
    struct io_watchdog_shared_info *  shared;
    char *                            shared_file;

    struct io_watchdog_ops ops;

    const char *program;

    int exact;
    int verbose;
};

/******************************************************************************
 *  Static global variables
 ******************************************************************************/

static struct io_watchdog_ctx ctx;

static struct io_watchdog_sym_info sym_info [] = {
    { "vfprintf",       (void **) &ctx.ops.vfprintf },
    { "vprintf",        (void **) &ctx.ops.vprintf },
    { "fputc",          (void **) &ctx.ops.fputc },
    { "fputs",          (void **) &ctx.ops.fputs },
    { "putc",           (void **) &ctx.ops.putc },
    { "putchar",        (void **) &ctx.ops.putchar },
    { "puts",           (void **) &ctx.ops.puts },
    { "write",          (void **) &ctx.ops.write },
    { "writev",         (void **) &ctx.ops.writev },
    { "pwrite",         (void **) &ctx.ops.pwrite },
    { "fwrite",         (void **) &ctx.ops.fwrite },
    { "fwrite_unlocked",(void **) &ctx.ops.fwrite_unlocked },
    { "fputs_unlocked", (void **) &ctx.ops.fputs_unlocked },
    { "_IO_putc",       (void **) &ctx.ops._IO_putc },
    { "_IO_puts",       (void **) &ctx.ops._IO_puts },
    { NULL, NULL}
};


/******************************************************************************
 *  Interposer Functions
 ******************************************************************************/

static int initialize_ops (struct io_watchdog_ctx *ctx)
{
    struct io_watchdog_sym_info *s = sym_info;

    while (s->name != NULL) {
        if (!(*s->symp = dlsym (ctx->libc, s->name))) {
            log_err ("Unable to find sym=[%s] in libc\n", s->name);
            return (-1);
        }
        s++;
    }
    return (0);
}

static int monitor_this_program ()
{
    const char *val;

    ctx.program = program_invocation_short_name;

    if (!(val = getenv ("IO_WATCHDOG_TARGET")))
        return (1);

    if (!program_invocation_name || !program_invocation_short_name)
        return (1);

    if (fnmatch (val, program_invocation_short_name, 0) == 0)
        return (1);

    return (fnmatch (val, program_invocation_name, 0) == 0);
}


static int process_env ()
{
    const char *val;

    ctx.verbose = 0;

    if ((val = getenv ("IO_WATCHDOG_SHARED_FILE"))) 
        ctx.shared_file = strdup (val);

    if ((val = getenv ("IO_WATCHDOG_DEBUG"))) {
        log_msg_set_verbose (atoi (val));
        ctx.verbose = 1;
    }

    if ((val = getenv ("IO_WATCHDOG_EXACT")) && val[0] != '0')
        ctx.exact = 1;

    return (0);
}

static void * find_libc ()
{
    glob_t gl;
    int i = 0;
    void *rv = NULL;

    if (glob ("/lib{64,}/libc.so*", GLOB_BRACE, NULL, &gl) != 0)
        return (NULL);

    while ((i < gl.gl_pathc) && !(rv = dlopen (gl.gl_pathv[i], RTLD_LAZY)))
            i++;

    globfree (&gl);

    return (rv);
}

static int log_output_fn (const char *buf)
{
    if (!ctx.ops.fputs)
        return (0);
    return ((*ctx.ops.fputs) (buf, stderr));
}

extern const char *io_watchdog_path;

static void spawn_watchdog ()
{
    pid_t pid;
    char *args [] = { "io-watchdog", "--server", "-F", NULL, NULL };

    unsetenv ("LD_PRELOAD");

    if ((pid = fork ()) < 0 || pid > 0)
        return;

    args [3] = strdup (ctx.shared_region->path);

    setenv ("IO_WATCHDOG_PROGRAM", ctx.program, 1);

    io_watchdog_shared_region_destroy (ctx.shared_region);

    if (execv (io_watchdog_path, args) < 0)
        log_err ("exec: %s: %s\n", io_watchdog_path, strerror (errno));
    exit (1);
}


void __attribute__ ((constructor)) io_watchdog_init (void)
{
    log_msg_init ("io-watchdog");
    log_msg_set_output_fn ((out_f) &log_output_fn);

    memset (&ctx, 0, sizeof (ctx));

    if ((ctx.libc = find_libc ()) == NULL) {
            /* Can't log the error here! */
            exit (15);
    }

    if (initialize_ops (&ctx) < 0)
        return;

    if (!monitor_this_program ())
        return;

    log_msg_set_secondary_prefix (ctx.program);

    if (process_env () < 0)
        return;

    ctx.shared_region = io_watchdog_shared_region_create (ctx.shared_file);
    if (!ctx.shared_region)
        log_err ("Failed to attach to shared memory\n");
    else
        ctx.shared = ctx.shared_region->shared;

    /* 
     *  If shared file was not set in env, 
     *    need to invoke server 
     */
    if (!ctx.shared_file) 
        spawn_watchdog ();

    ctx.shared->monitored_pid = getpid ();
    ctx.shared->started = 1;
    ctx.shared->start_time = time (NULL);
    strncpy (ctx.shared->cmd, ctx.program, sizeof (ctx.shared->cmd) - 1);

    unsetenv ("IO_WATCHDOG_SHARED_FILE");

    return;
}

static char * scale (unsigned long long n)
{
    static char buf [1024];
    double val = 0.0;
    int i = 0;
    unsigned long p = 1;
    char * suffixes [] = { " bytes", "K", "M", "G", "T", "P" };

    for (i = 0; i < 5; i++) {
        if ((int) (n / (p*=1024)) == 0)
            break;
    }

    val = ((double) n) / (p/1024.0);

    if (i > 1)
        snprintf (buf, sizeof (buf), "%.2f%s", val, suffixes [i]);
    else
        snprintf (buf, sizeof (buf), "%.0f%s", val, suffixes [i]);


    return (buf);

}

void __attribute__ ((destructor)) io_watchdog_exit (void)
{
    if (!ctx.shared_region)
        return;

    ctx.shared->exited = 1;

    if (ctx.shared->server_pid > (pid_t) 0)
        kill (ctx.shared->server_pid, SIGKILL);

    if (ctx.verbose) 
        log_verbose ("Process wrote %s\n", scale (ctx.shared->nbytes));

    io_watchdog_shared_region_destroy (ctx.shared_region);

    return;
}


/******************************************************************************
 *  Wrappers for libc functions that may call write(2).
 ******************************************************************************/

static inline void register_write_activity (int n)
{
    if (!ctx.shared)
        return;

    ctx.shared->flag = 1;
    if (n > 0)
        ctx.shared->nbytes += n;

    if (ctx.exact && (gettimeofday (&ctx.shared->lastio, NULL) < 0))
        log_debug ("gettimeofday: %s\n", strerror (errno));
}

#if defined vfprintf
#undef vfprintf
#endif
int vfprintf (FILE *fp, const char *fmt, va_list args)
{
    int rc = -1;
    rc = ((*ctx.ops.vfprintf) (fp, fmt, args));
    log_debug2 ("vfprintf (%p, \"%s\", ...) = %d\n", fp, fmt, rc);
    register_write_activity (rc);
    return (rc);
}

#if defined vprintf
#undef vprintf
#endif
int vprintf (const char *fmt, va_list args)
{
    int rc = ((*ctx.ops.vprintf) (fmt, args));
    log_debug2 ("vprintf (\"%s\", ...) = %d\n", fmt, rc);
    register_write_activity (rc);
    return (rc);
}

#if defined fprintf
#undef fprintf
#endif
int fprintf (FILE *fp, const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start (ap, fmt);
    rc = ((*ctx.ops.vfprintf) (fp, fmt, ap));
    va_end (ap);

    log_debug2 ("fprintf (%p, \"%s\", ...) = %d\n", fp, fmt, rc);
    register_write_activity (rc);

    return (rc);
}

#if defined printf
#undef printf
#endif
int printf (const char *fmt, ...)
{
    int rc;
    va_list ap;

    va_start (ap, fmt);
    rc = ((*ctx.ops.vprintf) (fmt, ap));
    va_end (ap);

    log_debug2 ("printf (\"%s\", ...) = %d\n", fmt, rc);
    register_write_activity (rc);

    return (rc);
}

int fputc (int c, FILE *s)
{
    int rc = ((*ctx.ops.fputc) (c, s));
    log_debug2 ("fputc (%c, %p) = %d\n", c, s, rc);
    register_write_activity (rc);
    return (rc);
}

int fputs (const char *s, FILE *fp)
{
    int rc = ((*ctx.ops.fputs) (s, fp));
    log_debug2 ("fputs (\"%s\", %p) = %d\n", s, fp, rc);
    register_write_activity (rc);
    return (rc);
}

#if defined putc
#undef putc
#endif
int putc (int c, FILE *s)
{
    int rc = ((*ctx.ops.putc) (c, s));
    log_debug2 ("putc (%c, %p) = %d\n", c, s, rc);
    register_write_activity (rc < 0 ? 0 : 1);
    return (rc);
}

int putchar (int c)
{
    int rc = ((*ctx.ops.putchar) (c));
    log_debug2 ("putchar (%c)\n", c);
    register_write_activity (rc < 0 ? 0 : 1);
    return (rc);
}

int puts (const char *s)
{
    int rc = ((*ctx.ops.puts) (s));
    log_debug2 ("puts (\"%s\") = %d\n", s, rc);
    register_write_activity (rc);
    return (rc);
}

ssize_t write (int fd, const void *buf, size_t count)
{
    ssize_t rc = ((*ctx.ops.write) (fd, buf, count));
    log_debug2 ("write (%d, %p, %ld) = %ld\n", fd, buf, count, rc);
    register_write_activity (rc);
    return (rc);
}

ssize_t writev (int fd, const struct iovec *v, int count)
{
    ssize_t rc = ((*ctx.ops.writev) (fd, v, count));
    log_debug2 ("writev (%d, %p, %ld) = %ld\n", fd, v, count, rc);
    register_write_activity (rc);
    return (rc);
}

ssize_t pwrite (int fd, const void *buf, size_t s, off_t off)
{
    ssize_t rc = ((*ctx.ops.pwrite) (fd, buf, s, off));
    log_debug2 ("pwrite (%d, %p, %lx) = %ld\n", fd, buf, s, off, rc);
    register_write_activity (rc);
    return (rc);
}

size_t fwrite (const void *ptr, size_t size, size_t nmemb, FILE *fp)
{
    size_t rc = ((*ctx.ops.fwrite) (ptr, size, nmemb, fp));
    log_debug2 ("write (%p, %ld, %ld, %p) = %ld\n", ptr, size, nmemb, fp, rc);
    register_write_activity (rc);
    return (rc);
}

#if defined fwrite_unlocked
#undef fwrite_unlocked
#endif
size_t fwrite_unlocked (const void *ptr, size_t size, size_t n, FILE *fp)
{
    size_t rc = ((*ctx.ops.fwrite_unlocked) (ptr, size, n, fp));
    log_debug2 ("fwrite_unlocked (%p, %ld, %ld, %p) = %d\n", ptr, size, n, fp, rc);
    register_write_activity (rc);
    return (rc);
}

int fputs_unlocked (const char *s, FILE *fp)
{
    size_t rc = ((*ctx.ops.fputs_unlocked) (s, fp));
    log_debug2 ("fwrite_unlocked (%s, %p) = %d\n", s, fp, rc);
    register_write_activity (rc);
    return (rc);
}

int _IO_putc (int c, FILE *s)
{
    int rc = ((*ctx.ops._IO_putc) (c, s));
    log_debug2 ("_IO_putc (%c, %p) = %d\n", c, s, rc);
    register_write_activity (rc < 0 ? 0 : 1);
    return (rc);
}

int _IO_puts (const char *s)
{
    int rc = ((*ctx.ops._IO_puts) (s));
    log_debug2 ("_IO_puts (\"%s\") = %d\n", s, rc);
    register_write_activity (rc);
    return (rc);
}

/*
 * vi: ts=4 sw=4 expandtab
 */
