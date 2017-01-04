#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <unistd.h>
#include <yaml.h>
#include "hiredis.h"

#include "ts_logging.h"
#include "ts_server.h"
#include "ts_sentinel.h"

struct tsConf {
  ts_server *server;
  char *nc_conf_file;
  char *nc_service_name;
  char *nc_log_file;
};

typedef struct tsConf ts_conf;

ts_conf* tc_conf_init(void) {
  ts_conf *tc = calloc(1, sizeof(ts_conf));
  ts_server *ts = calloc(1, sizeof(ts_server));
  tc->server = ts;
  return tc;
}

void parseConfig(int argc, char **argv, ts_conf *ts_conf);

void main(int argc, char **argv) {

  ts_conf *config = tc_conf_init();
  
  parseConfig(argc, argv, config);

  redisContext *redis_ctx;
  
  redis_ctx = ts_sentinel_connect(config->server);

  ts_servers servers;

  ts_servers *sPtr = &servers;

  ts_sentinel_set_masters(redis_ctx, sPtr);
  
  ts_sentinel_disconnect(redis_ctx);

  ts_sentinel_subscribe(config->server);
}

void parseConfig(int argc, char **argv, ts_conf *ts_conf) {

  char logging[200];

  int index;
  int c;

  opterr = 0;
  
  while ((c = getopt (argc, argv, "h:p:f:c:l:")) != -1) {
    switch (c)
    {
      case 'h':
        ts_conf->server->host = optarg;
        break;
      case 'p':
        ts_conf->server->port = atoi(optarg);
        break;
      case 'f':
        ts_conf->nc_conf_file = optarg;
        break;
      case 'c':
        ts_conf->nc_service_name = optarg;
        break;
      case 'l':
        ts_conf->nc_log_file = optarg;
        break;
      case '?':
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return;
      default:
        abort();
    }
  }

  if(ts_conf->server->host == NULL) {
    fprintf(stderr, "-h (nc_host) flag is required\n");
    exit(1);
  }
  else if(ts_conf->server->port == 0) {
    fprintf(stderr, "-p (nc_port) flag is required\n");
    exit(1);
  }
  else if(ts_conf->nc_conf_file == NULL) {
    fprintf(stderr, "-f (nc_conf_file) flag is required\n");
    exit(1);
  }
  else if(ts_conf->nc_service_name == NULL) {
    fprintf(stderr, "-c (nc_service_name) flag is required\n");
    exit(1);
  }
  else {
    sprintf(logging, "ip: %s \nport: %d\ntwemproxy_config_path: %s\ntwemproxy_service_name: %s\nlog_file_location: %s\n", 
     ts_conf->server->host, ts_conf->server->port, ts_conf->nc_conf_file, ts_conf->nc_service_name, ts_conf->nc_log_file);
    printf("%s\n", logging);
    ts_log_info(logging);
  }
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
