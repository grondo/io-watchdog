INCLUDES = \
    -I$(top_srcdir)/src/lib

lib_LTLIBRARIES = \
    io-watchdog-interposer.la

io_watchdog_interposer_la_LDFLAGS = \
    -Wl,--version-script=version.map \
    -module \
    -avoid-version

io_watchdog_interposer_la_LIBADD = \
    -ldl \
    $(top_srcdir)/src/lib/log_msg.lo \
    $(top_srcdir)/src/lib/shared.lo 


io_watchdog_interposer_la_SOURCES = \
    io-watchdog-interposer.c
