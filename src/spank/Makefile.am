INCLUDES = \
    -I$(top_srcdir)/src/lib \
    -I$(top_srcdir)/src/conf

pkglib_LTLIBRARIES = \
    io-watchdog.la

io_watchdog_la_LDFLAGS = \
    -Wl,--version-script=version.map \
    -module \
    -avoid-version

io_watchdog_la_LIBADD = \
    $(top_srcdir)/src/conf/libconf.la


io_watchdog_la_SOURCES = \
    io-watchdog.c
