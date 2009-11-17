#!/bin/sh

WAIT_TIME=22

echo "Checking basic timeout functionality of io-watchdog..."

$WATCHDOG --action=${ACTIONDIR}/kill-process --timeout=1s sleep 30 &
PID=$!

#
#  sleep for WAIT_TIME seconds, then kill PID with SIGALRM
#
(sleep $WAIT_TIME; echo $PID timed out, killing it; kill -ALRM $PID ) &
SLEEP_PID=$!

wait $PID 2>&1 >/dev/null

if [ $? -eq 142 ]; then
   echo "Error: Command killed by SIGALRM" >&2
   exit 1
fi

#
#  Kill sleeper subshell if it is still around
#
for p in $(pgrep -P $SLEEP_PID) $SLEEP_PID; do
  kill $p 2>&1
done

exit 0
