%{
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "conf.h"

#define YYSTYPE char *
#include "conf-parser.h"

%}

%option nounput

%%

#[^\n]*         ; /* Ignore comments.   */
[ \t\r]+        ; /* Ignore whitespace. */
;               { return ';'; }
,               { return ','; }
(#.*)?\\?\n     { return '\n'; }

\"[^\"]*\" |
\'[^\']*\'      {
                        yytext [strlen (yytext) - 1] = '\0';
                        yylval = strdup (yytext+1);
                        return STRING;
                }

search(path)?   { return SEARCH; }
program         { return PROGRAM; }
action          { return ACTION; }
actions         { return ACTIONS; }
timeout         { return TIMEOUT; }
rank            { return RANK; }
target          { return TARGET; }
exact-timeout   { return EXACT_TIMEOUT; }
=               { return '='; }

[^=;, \t\r\n]+  {
                        yylval = strdup (yytext);
                        return STRING;
                }

%%


/*
 * vi: ts=4 sw=4 expandtab
 */
