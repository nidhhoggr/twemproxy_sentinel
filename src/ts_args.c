#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ts_logging.h"
#include "ts_server.h"
#include "ts_args.h"

ts_args* tc_args_init(void) {
  ts_args *tc = calloc(1, sizeof(ts_args));
  ts_server *ts = calloc(1, sizeof(ts_server));
  tc->server = ts;
  return tc;
}

void ts_args_free(ts_args *config) { 
  free(config->server);
  free(config);
}

void ts_args_parse(int argc, char **argv, ts_args *ts_args) {

  char logging[200];

  int index;
  int c;

  opterr = 0;
  
  while ((c = getopt (argc, argv, "h:p:f:c:l:")) != -1) {
    switch (c)
    {
      case 'h':
        ts_args->server->host = optarg;
        break;
      case 'p':
        ts_args->server->port = atoi(optarg);
        break;
      case 'f':
        ts_args->nc_conf_file = optarg;
        break;
      case 'c':
        ts_args->nc_service_name = optarg;
        break;
      case 'l':
        ts_args->nc_log_file = optarg;
        break;
      case '?':
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return;
      default:
        abort();
    }
  }

  if(ts_args->server->host == NULL) {
    fprintf(stderr, "-h (nc_host) flag is required\n");
    exit(1);
  }
  else if(ts_args->server->port == 0) {
    fprintf(stderr, "-p (nc_port) flag is required\n");
    exit(1);
  }
  else if(ts_args->nc_conf_file == NULL) {
    fprintf(stderr, "-f (nc_conf_file) flag is required\n");
    exit(1);
  }
  else if(ts_args->nc_service_name == NULL) {
    fprintf(stderr, "-c (nc_service_name) flag is required\n");
    exit(1);
  }
  else {
    sprintf(logging, "ip: %s \nport: %d\ntwemproxy_config_path: %s\ntwemproxy_service_name: %s\nlog_file_location: %s\n", 
     ts_args->server->host, ts_args->server->port, ts_args->nc_conf_file, ts_args->nc_service_name, ts_args->nc_log_file);
    printf("%s\n", logging);
    ts_log_info(logging);
  }
}
