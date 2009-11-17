#!/bin/sh

set +b

. ${SRCDIR}/functions

NITERS=${TEST_ITERATIONS-1}
WATCHDOG=${TOP_BUILDDIR}/src/watchdog/io-watchdog
TARGET=lt-get-set-timeout

log_msg "Checking io_watchdog_get/set_timeout() (iterations=${NITERS})..."

for i in `seq 1 ${NITERS}`; do
   $WATCHDOG --target=${TARGET} ${V} -t 10 ./get-set-timeout ${V} -t 10
   if [ $? -ne 0 ]; then
      log_fatal "Watchdog exited with code=$?"
   fi
   log_verbose "Run $i Successful."
done

exit 0
