#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ts_logging.h"
#include "ts_server.h"
#include "ts_sentinel.h"
#include "ts_args.h"
#include "ts_nc_config.h"


void main(int argc, char **argv) {
  
  ts_args *tsArgs = tc_args_init();
  
  ts_args_parse(argc, argv, &tsArgs);

  redisContext *redis_ctx;
  
  redis_ctx = ts_sentinel_connect(&tsArgs->server);

  ts_servers *servers = ts_sentinel_get_masters(&redis_ctx);
 
  ts_nc_config_update(&tsArgs, &servers);

  ts_sentinel_disconnect(&redis_ctx);

  ts_sentinel_subscribe(&tsArgs);

  ts_args_free(&tsArgs);
}

int execute_service_restart(char *service_name) {
  pid_t pid;
  int status;
  if ((pid = fork()) == 0) {
    /* the child process */
    execlp("service", "service", service_name, "restart", NULL);
    /* if execl() was successful, this won't be reached */
    _exit(0);
  }

  if ((pid > 0) && (waitpid(pid, &status, 0) > 0) && (WIFEXITED(status) && !WEXITSTATUS(status))) {
    return 1;
  }

  return 0;
}
