
include_HEADERS = io-watchdog.h
lib_LTLIBRARIES = libio-watchdog.la
man_MANS =        io-watchdog.3

libio_watchdog_la_LDFLAGS = \
	-version-info $(META_LT_CURRENT):$(META_LT_REVISION):$(META_LT_AGE) \
	-export-symbols-regex "io_watchdog_.*"

libio_watchdog_la_SOURCES = \
	libio-watchdog.c
	io-watchdog.h
