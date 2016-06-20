#!/bin/sh

. ${SRCDIR}/functions

WAIT_TIME=22

log_msg "Checking basic timeout functionality of io-watchdog..."

exec 3>&2
[ ${VERBOSE-0} -lt 2 ] && exec >/dev/null 2>/dev/null 
$WATCHDOG --action=${ACTIONDIR}/kill-process --timeout=1s sleep 30 &
PID=$!

#
#  sleep for WAIT_TIME seconds, then kill PID with SIGALRM
#
(sleep $WAIT_TIME; log_msg $PID timed out, killing it; kill -ALRM $PID ) &
SLEEP_PID=$!

wait $PID 2>&1 >/dev/null

if [ $? -eq 142 ]; then
   exec 2>&3
   die "Error: Command killed by SIGALRM"
fi

#
#  Kill sleeper subshell if it is still around
#
for p in $(pgrep -P $SLEEP_PID) $SLEEP_PID; do
  kill $p 2>&1
done

exit 0
