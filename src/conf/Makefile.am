INCLUDES = \
	-I$(top_srcdir)/src/lib

man_MANS = \
    io-watchdog.conf.5

noinst_LTLIBRARIES = \
	libconf.la

libconf_la_SOURCES = \
	conf-parser.c \
	conf-parser.h \
	conf-lexer.c \
	conf.h \
	conf.c

nodist_libconf_la_SOURCES = \
	paths.c

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

paths.c : $(top_srcdir)/META $(top_builddir)/config.h
	@(echo "const char *io_watchdog_path = \"$(bindir)/io-watchdog\";" \
	 )> paths.c

EXTRA_DIST = \
	conf-parser.l \
	conf-parser.y

CLEANFILES = \
	conf-parser.h \
	conf-parser.c \
	conf-lexer.c \
	paths.c
	
