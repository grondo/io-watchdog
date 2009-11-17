#!/bin/sh

NITERS=${TEST_ITERATIONS-1}
WATCHDOG=${TOP_BUILDDIR}/src/watchdog/io-watchdog
VERBOSE=${DEBUG+-vvv}
TARGET=lt-get-set-timeout

echo "Checking io_watchdog_get/set_timeout() (iterations=${NITERS})..."

for i in `seq 1 ${NITERS}`; do
   $WATCHDOG --target=${TARGET} ${VERBOSE} -t 10 ./get-set-timeout ${VERBOSE} -t 10
   if [ $? -ne 0 ]; then
      exit 1
   fi
   echo "Run $i Successful."
done
