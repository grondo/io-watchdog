INCLUDES = \
    -I $(top_srcdir)/src/lib \
    -I $(top_srcdir)/src/conf

bin_PROGRAMS = \
    io-watchdog

io_watchdog_SOURCES = \
    io-watchdog.c

io_watchdog_LDFLAGS = \
    $(top_srcdir)/src/lib/libcommon.la \
    $(top_srcdir)/src/conf/libconf.la
