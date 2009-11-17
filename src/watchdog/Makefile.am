INCLUDES = \
    -I $(top_srcdir)/src/lib \
    -I $(top_srcdir)/src/conf

bin_PROGRAMS = \
    io-watchdog

man_MANS = \
    io-watchdog.1

io_watchdog_SOURCES = \
    io-watchdog.c

io_watchdog_LDFLAGS = \
    $(top_builddir)/src/lib/libcommon.la \
    $(top_builddir)/src/conf/libconf.la \
	$(LIBPTHREAD)
