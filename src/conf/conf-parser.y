%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "conf.h"
#include "list.h"
#include "log_msg.h"

extern int yylex ();
void yyerror (const char *s);
extern FILE *yyin;

static int io_watchdog_conf_line;

#define YYSTYPE char *

#define YYDEBUG 1
int yydebug = 0;

static int cf_action (char * name, char *val);
static int cf_program (char *program);
static int cf_timeout (char *timeout);
static int cf_rank (char *rank);
static int cf_target (char *target);
static int cf_actions ();
static int cf_path_append ();
static int stringlist_append (char *string);


%}


%token STRING
%token PROGRAM
%token ACTION
%token ACTIONS
%token TIMEOUT
%token RANK
%token TARGET
%token SEARCH


%%

file    : /* empty */
        | file stmts
        ;

stmts   : end 
        | stmt end
        | stmts stmt
        | search
        | actions
        ;

stmt    : ACTION STRING '=' STRING   { if (cf_action ($2, $4) < 0)   YYABORT; }
        | PROGRAM STRING             { if (cf_program ($2) < 0)      YYABORT; }
        | TIMEOUT '=' STRING         { if (cf_timeout ($3) < 0)      YYABORT; }
        | RANK '=' STRING            { if (cf_rank ($3) < 0)         YYABORT; }
        | TARGET '=' STRING          { if (cf_target ($3) < 0)       YYABORT; }
        ;

actions : ACTIONS '=' list           { if (cf_actions () < 0)        YYABORT; }
          end
        ;

search  : SEARCH list                { if (cf_path_append () < 0)    YYABORT; }
          end                       
        ;

list    : STRING                     { stringlist_append ($1); }
        | list STRING                { stringlist_append ($2); }
        | list ',' STRING            { stringlist_append ($3); }
        ;

end     : '\n'                       { io_watchdog_conf_line++; }
        | ';'
        ;

%%

static io_watchdog_conf_t conf;
static const char * io_watchdog_conf_file = NULL;
static List stringlist = NULL;

void io_watchdog_conf_debug ()
{
    yydebug = 1;
}

static int stringlist_append (char *str)
{
    if (stringlist == NULL)
        stringlist = list_create (NULL);
    list_append (stringlist, str);
    return (0);
}

void yyerror (const char *s)
{
    log_err ("%d: %s\n", io_watchdog_conf_line, s);
}

static const char * cf_file ()
{
    if (!io_watchdog_conf_file)
        return ("stdin");
    return (io_watchdog_conf_file);
}

static int cf_line () {
    return (io_watchdog_conf_line);
}

int io_watchdog_conf_parse (io_watchdog_conf_t cf, const char *path)
{
    io_watchdog_conf_file = NULL;

    if (strcmp (path, "-") == 0)
        yyin = stdin;
    else if (!(yyin = fopen (path, "r"))) {
        int err = errno;
        log_debug ("open: %s: %s\n", path, strerror (errno));
        errno = err;
        return (-1);
    }

    io_watchdog_conf_file = path;
    io_watchdog_conf_line = 1;

    conf = cf;

    log_verbose ("reading config from \"%s\"\n", cf_file ());

    if (yyparse ()) {
        log_err ("%s: %d: parser failed\n", cf_file (), cf_line ());
        errno = 0;
        return (-1);
    }

    io_watchdog_conf_set_current_program (conf, "default");

    fclose (yyin);

    return (0);
}

static int cf_action (char *name, char *val)
{
    int rc = io_watchdog_conf_append_action (conf, name, val);
    log_debug2 ("%s: %d: action %s = %s\n", cf_file (), cf_line (), name, val);
    free (name);
    free (val);
    return (rc);
}

static int cf_program (char *program)
{
    int rc = io_watchdog_conf_new_program (conf, program);
    log_debug2 ("%s: %d: new target prog %s\n", cf_file (), cf_line (), program);
    free (program);
    return (rc);
}

static int cf_timeout (char *timeout)
{
    int rc = io_watchdog_conf_set_timeout (conf, timeout);
    log_debug2 ("%s: %d: timeout = %s\n", cf_file (), cf_line (), timeout);
    free (timeout);
    return (rc);
}

static int cf_rank (char *rank)
{
    long l;
    char *p;

    l = strtol (rank, &p, 10);
    if ((*p != '\0') || (l < 0)) {
        log_err ("%s: %d: Invalid rank `%s'\n", cf_file (), cf_line (), rank);
        return (-1);
    }

    log_debug2 ("%s: %d: target rank = %s\n", cf_file (), cf_line (), rank);

    free (rank);

    return (io_watchdog_conf_set_rank (conf, (int) l));
}

static int cf_target (char *target)
{
    int rc = io_watchdog_conf_set_target (conf, target);
    log_debug2 ("%s: %d: target = %s\n", cf_file (), cf_line (), target);
    free (target);
    return (rc);
}

static int cf_actions ()
{
    char *s;

    while ((s = list_pop (stringlist))) {
        log_debug2 ("%s: %d: append action %s\n", cf_file (), cf_line (), s);
        io_watchdog_conf_set_actions (conf, s);
    }
    return (0);
}

static int cf_path_append ()
{
    char *s;

    while ((s = list_pop (stringlist))) {
        log_debug2 ("%s: %d: append path %s\n", cf_file (), cf_line (), s);
        io_watchdog_conf_append_action_path (conf, s);
        free (s);
    }

    return (0);
}



/*
 * vi: ts=4 sw=4 expandtab
 */
