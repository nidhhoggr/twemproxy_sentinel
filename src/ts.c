#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ts_logging.h"
#include "ts_server.h"
#include "ts_sentinel.h"
#include "ts_args.h"
#include "ts_nc_config.h"


void main(int argc, char **argv) {
  
  char host_str[128] = "redis-004 10.132.16.48 6382 10.132.169.170 6382";

  ts_sentinel_parse_master_promotion(host_str);
  
  ts_args *tsArgs = tc_args_init();
  
  ts_args_parse(argc, argv, &tsArgs);

  redisContext *redis_ctx;
  
  redis_ctx = ts_sentinel_connect(&tsArgs->server);

  ts_servers *servers = ts_sentinel_get_masters(&redis_ctx);
 
  ts_nc_config_update(&tsArgs, &servers);

  ts_sentinel_disconnect(&redis_ctx);

  //ts_sentinel_subscribe(tsArgs->server);

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
