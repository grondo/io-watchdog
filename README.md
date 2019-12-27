### io-watchdog - The IO Watchdog.

The IO Watchdog is a facility for monitoring user applications,
most notably parallel jobs, for "hangs" which typically have
a side-effect of ceasing all write activity (IO) in a cyclic
application (i.e. an application that writes something to a log
or data file during each cycle of computation). The io-watchdog
attempts to monitor all write activity coming from an application
and triggers a set of user-defined actions when IO has ceased for
a configurable timeout period.

The IO watchdog consists of a LD_PRELOAD library (the interposer)
which intercepts calls to various output-related calls in libc,
along with a watchdog server which wakes up periodically and
ensures that the application has written something during the last
timeout period. If not, the watchdog server issues a warning on
the application's stderr, and invokes all user defined actions,
which could include running a debugger on the application, sending
email to the user, etc.

Set up of the LD_PRELOAD library is facilitated with either the
io-watchdog(1) utility, or a SPANK plug-in for SLURM which adds
a new --io-watchdog command line option to srun(1).  To enable
the io-watchdog SLURM plugin, the following line must exist in
/etc/slurm/plugstack.conf:

```
 required io-watchdog.so
```

The io-watchdog supports the following tunable parameters:

timeout 
:The watchdog timeout. Default = 1 hour.
rank
:The MPI rank for which the watchdog runs if a SLURM job.
actions
:A list of actions to run on watchdog trigger.
target
:A pattern match for target of io-watchdog if running multiple applications in a pipeline or single job.

These may be set on the command line, or in an io-watchdog configuration
file. Configuration files that are read automatically if they exist
are

  * `/etc/io-watchdog/io-watchdog.conf`    System defaults
  * `~/.io-watchdogrc`                     User defaults.

A config file may also be specified on the command line to override
the default location of the user configuration.

See `io-watchdog --help` and `srun --io-watchdog=help` for
more information.
