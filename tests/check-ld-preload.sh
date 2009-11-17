#!/bin/sh

. ${SRCDIR}/functions

NITERS=${TEST_ITERATIONS-1}
INTERPOSER=${TOP_BUILDDIR}/src/interposer/.libs/io-watchdog-interposer.so
export IO_WATCHDOG_TIMEOUT=10
export IO_WATCHDOG_TARGET=lt-get-set-timeout
export IO_WATCHDOG_SERVER_PATH=$WATCHDOG
export IO_WATCHDOG_DEBUG=$((${VERBOSE-2}-2))

log_msg "Checking io_watchdog_get/set_timeout() from direct LD_PRELOAD..."
for i in `seq 1 ${NITERS}`; do
   LD_PRELOAD=${INTERPOSER} ./get-set-timeout ${V} -t 10
   if [ $? -ne 0 ]; then
      exit 1
   fi
   log_verbose "Run $i Successful."
done
exit 0
