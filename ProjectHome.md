io-watchdog is a facility for monitoring user applications and
parallel jobs for "hangs" which typically have a side effect
of ceasing all IO in a cyclic application (i.e. one that
writes something to a log or data file during each cycle of
computation). The io-watchdog attempts to watch all IO coming
from an application and triggers a set of user-defined actions
when IO has stopped for a configurable timeout period.

Read the full IO Watchdog [README](http://io-watchdog.googlecode.com/git/README)

---

## NEWS ##
### io-watchdog v0.8 released ###
2010-08-23

This version fixes a critical bug in the io-watchdog plugin for SLURM
which caused the `--io-watchdog` option to srun to fail to work for
all jobs unless the `rank` option was used (e.g. `--io-watchdog rank=0`).

All users of the io-watchdog plugin for SLURM should update to this release
immediately.

---

### io-watchdog v0.7 released ###
2009-11-17

The major feature in this new version of **io-watchdog** is the addition of a client
API to get and set the current watchdog timeout. See the `io-watchdog(3)` manpage
for more information about using the `libio-watchdog` library.

Other changes in this release include

  * Add -q, --quiet option to **io-watchdog**
  * Set `IO_WATCHDOG_TIMEOUT` and `IO_WATCHDOG_TARGET` in the environment of action scripts.
  * Fix for hangs in **io-watchdog** on `exec`(2) failures
  * The **io-watchdog** now exits by default after the first timeout event is reached. This means that if the monitored process is not terminated by any action script, then it will continue to run unmonitored by the watchdog. There is a new option to `io-watchdog`(1), `--persistent` which restores the old behavior.
  * The spec file included with the distribution now splits the io-watchdog RPM into a main package and -libs, -devel, and -slurm subpackages.
  * A small testsuite is now included with the **io-watchdog** source.

---
