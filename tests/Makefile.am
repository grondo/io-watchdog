
CPPFLAGS =      -I$(top_srcdir)/src/api -I$(top_srcdir)/src/lib
CFLAGS =        -g -Wall -Werror

TESTS_ENVIRONMENT = \
	LD_LIBRARY_PATH=$(top_builddir)/src/interposer/.libs \
	SRCDIR=$(srcdir) \
	TOP_SRCDIR=$(top_srcdir) \
	BUILDDIR=$(builddir) \
	TOP_BUILDDIR=$(top_builddir) \
	WATCHDOG=$(top_builddir)/src/watchdog/io-watchdog \
	ACTIONDIR=$(srcdir)/actions

TESTS = \
	check-timeout.sh \
	check-get-set-timeout.sh \
	check-action-environment.expect

check_PROGRAMS = \
	get-set-timeout

get_set_timeout_SOURCES = \
	$(top_srcdir)/src/lib/log_msg.c \
	$(top_srcdir)/src/lib/log_msg.h \
	$(srcdir)/get-set-timeout.c

get_set_timeout_LDADD = \
	$(top_builddir)/src/api/libio-watchdog.la
