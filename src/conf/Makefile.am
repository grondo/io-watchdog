INCLUDES = \
	-I$(top_srcdir)/src/lib

noinst_LTLIBRARIES = \
	libconf.la


libconf_la_SOURCES = \
	conf-parser.c \
	conf-parser.h \
	conf-lexer.c \
	conf.h \
	conf.c

libconf_la_LIBADD = \
	$(top_srcdir)/src/lib/libcommon.la -lfl 

noinst_PROGRAMS = \
	test

test_SOURCES = \
	main.c

test_LDADD = \
	$(top_srcdir)/src/lib/libcommon.la \
	$(srcdir)/libconf.la 

conf-lexer.c : conf-parser.l
	$(LEX) -oconf-lexer.c conf-parser.l

conf-parser.c : conf-parser.y
	$(YACC) -d -oconf-parser.c conf-parser.y

EXTRA_DIST = \
	conf-parser.l \
	conf-parser.y

CLEANFILES = \
	conf-parser.h \
	conf-parser.c \
	conf-lexer.c
