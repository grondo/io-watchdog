INCLUDES = \
    -I$(top_srcdir)/src/lib \
    -I$(top_srcdir)/src/api

lib_LTLIBRARIES = \
    io-watchdog-interposer.la

io_watchdog_interposer_la_LDFLAGS = \
    -Wl,--version-script=version.map \
    -module \
    -avoid-version

io_watchdog_interposer_la_LIBADD = \
    -ldl \
    $(top_srcdir)/src/lib/log_msg.lo \
    $(top_srcdir)/src/lib/shared.lo \
    $(top_srcdir)/src/conf/paths.lo \
	$(LIBPTHREAD)


io_watchdog_interposer_la_SOURCES = \
    io-watchdog-interposer.c
