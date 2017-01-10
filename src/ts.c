#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ts_logging.h"
#include "ts_server.h"
#include "ts_sentinel.h"
#include "ts_args.h"
#include "ts_nc_config.h"

void ts_nc_update_masters_and_restart(ts_args **tsArgs);
static int ts_nc_service_restart(char *service_name);

static redisContext *redis_ctx;

void main(int argc, char **argv) {
  
  ts_args *tsArgs = ts_args_init();
  
  ts_args_parse(argc, argv, &tsArgs);

  redis_ctx = ts_sentinel_connect(&tsArgs->server);

  ts_servers *servers = ts_sentinel_get_masters(&redis_ctx);
  
  ts_nc_config_update(&tsArgs, &servers);

  ts_nc_service_restart(tsArgs->nc_service_name);

  ts_sentinel_subscribe(&tsArgs);
}

void ts_nc_update_masters_and_restart(ts_args **tsArgs) {
  
  redisReconnect(redis_ctx); 

  ts_servers *servers = ts_sentinel_get_masters(&redis_ctx);
  
  ts_nc_config_update(tsArgs, &servers);
  
  ts_nc_service_restart((*tsArgs)->nc_service_name);
}

static int ts_nc_service_restart(char *service_name) {
  pid_t pid;
  int status;
  
  if ((pid = fork()) == 0) {
    /* the child process */
    execlp("service", "service", service_name, "restart", NULL);
    /* if execl() was successful, this won't be reached */
    _exit(0);
  }
  if ((pid > 0) && (waitpid(pid, &status, 0) > 0)) {
    if (WIFEXITED(status) && !WEXITSTATUS(status)) {
      return 1;
    }
  }
  return 0;
}
