#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <yaml.h>

#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include "ts_logging.h"
#include "ts_server.h"

struct tsConf {
  char *nc_host;
  uint16_t nc_port;
  char *nc_conf_file;
  char *nc_service_name;
  char *nc_log_file;
};

//typedef struct tsConf ts_conf;

void parseConfig(int argc, char **argv, struct tsConf *ts_conf);

void main(int argc, char **argv) {

  //parseyaml();

  struct tsConf ts_conf = {};

  parseConfig(argc, argv, &ts_conf);

  int restart_status = 0;

  ts_servers servers;

  ts_servers *sPtr = &servers;

  setRedisMasterServers(&ts_conf, sPtr);

  subscribeToSentinelChannel(&ts_conf);

  
  //restart_status = execute_service_restart(ts_conf.nc_service_name);

  //if(restart_status == 0) {
  //  printf("restart failed");
  //}
}

void parseConfig(int argc, char **argv, struct tsConf *ts_conf) {

  char logging[200];

  int index;
  int c;

  opterr = 0;
  
  while ((c = getopt (argc, argv, "h:p:f:c:l:")) != -1) {
    switch (c)
    {
      case 'h':
        ts_conf->nc_host = optarg;
        break;
      case 'p':
        ts_conf->nc_port = atoi(optarg);
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

  if(ts_conf->nc_host == NULL) {
    fprintf(stderr, "-h (nc_host) flag is required\n");
    exit(1);
  }
  else if(ts_conf->nc_port == 0) {
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
     ts_conf->nc_host, ts_conf->nc_port, ts_conf->nc_conf_file, ts_conf->nc_service_name, ts_conf->nc_log_file);
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

void onPublishMessage(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;
  int j;

  if (reply == NULL) return;

  if (r->type == REDIS_REPLY_ARRAY) {
    for (j = 0; j < r->elements; j++) {
      printf("%u) %s\n", j, r->element[j]->str);
    }
  }
}

int setRedisMasterServers(struct tsConf *ts_conf, ts_servers *servers) {
  
  redisContext *c;
  redisReply *reply;
  int j, k;
 
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds

  c = redisConnectWithTimeout(ts_conf->nc_host, ts_conf->nc_port, timeout);
  if (c == NULL || c->err) {
    if (c) {
      printf("Connection error: %s\n", c->errstr);
      redisFree(c);
    } else {
      printf("Connection error: can't allocate redis context\n");
    }
    exit(1);
  }

  reply = redisCommand(c,"SENTINEL %s","masters");
  printf("PING: %s\n", reply->str);
  if (reply->type == REDIS_REPLY_ARRAY) {
    for (j = 0; j < reply->elements; j++) {
      ts_server server;
      server.name = reply->element[j]->element[1]->str;
      server.host = reply->element[j]->element[3]->str;
      server.port = reply->element[j]->element[5]->str;
      ts_server *sPtr = &server;
      servers = ts_add_server(servers, sPtr);
      const char *server_fqn = ts_set_server_fqn(sPtr);
      printf("%s\n", server_fqn);
    }
  }

  freeReplyObject(reply);

  /* Disconnects and frees the context */
  redisFree(c);

  return 0;
}

int subscribeToSentinelChannel(struct tsConf *ts_conf) {

  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  redisAsyncContext *c = redisAsyncConnect(ts_conf->nc_host, ts_conf->nc_port);
  if (c->err) {
    printf("error: %s\n", c->errstr);
    return 1;
  }

  redisLibeventAttach(c, base);
  redisAsyncCommand(c, onPublishMessage, NULL, "SUBSCRIBE +switch-master");
  event_base_dispatch(base);

  return 0;
}
