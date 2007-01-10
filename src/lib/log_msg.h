#ifndef _LOG_MSG_H
#define _LOG_MSG_H

typedef int (*out_f) (const char *buf);

int log_msg_init (const char *prefix);
void log_msg_fini ();

int log_msg_verbose ();
int log_msg_set_verbose (int level);
void log_msg_set_secondary_prefix (const char *prefix);
void log_msg_set_output_fn (out_f out);
int log_msg_quiet ();
void log_fatal (int err, const char *format, ...);
int log_err (const char *format, ...);
void log_msg (const char *format, ...);
void log_verbose (const char *format, ...);
void log_debug (const char *format, ...);
void log_debug2 (const char *format, ...);
void log_debug3 (const char *format, ...);

#endif /* !_LOG_MSG_H */
