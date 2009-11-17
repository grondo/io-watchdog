#!/bin/sh

. ${SRCDIR}/functions

NITERS=$((${TEST_ITERATIONS-1} * 1024))
WATCHDOG=${TOP_BUILDDIR}/src/watchdog/io-watchdog
TARGET=lt-api-stress

log_msg "API stress (iterations=${NITERS})..."

$WATCHDOG --target=${TARGET} ${V} ./api-stress -n ${NITERS} || \
				   die "API stress failed"

log_verbose "Success."

exit 0
