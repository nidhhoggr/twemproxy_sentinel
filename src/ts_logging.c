#include <stdio.h>
#include <unistd.h>
#include <syslog.h>

void ts_log_info(char *msg) {
 openlog("twemproxy_sentinel", LOG_PID|LOG_CONS, LOG_DAEMON);
 syslog(LOG_INFO, msg);
 closelog();
}

void ts_log_warning(char *msg) {
 openlog("twemproxy_sentinel", LOG_PID|LOG_CONS, LOG_DAEMON);
 syslog(LOG_WARNING, msg);
 closelog();
}
