EXTRA_DIST = \
	version.map

INCLUDES = \
    -I$(top_srcdir)/src/lib \
    -I$(top_srcdir)/src/conf

pkglib_LTLIBRARIES = \
    io-watchdog.la

io_watchdog_la_LDFLAGS = \
    -Wl,--version-script=$(srcdir)/version.map \
    -module \
    -avoid-version

io_watchdog_la_LIBADD = \
    $(top_builddir)/src/conf/libconf.la


io_watchdog_la_SOURCES = \
    io-watchdog.c
