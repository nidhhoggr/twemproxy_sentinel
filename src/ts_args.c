#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

#include "ts_server.h"
#include "ts_args.h"

ts_args* ts_args_init(void) {
  ts_args *tsArgs = calloc(1, sizeof(ts_args));
  ts_server *tsServer = calloc(1, sizeof(ts_server));
  tsArgs->server = tsServer;
  tsArgs->nc_channel_name = "+switch-master";
  tsArgs->nc_log_file = "twemproxy-sentinel";
  return tsArgs;
}

void ts_args_free(ts_args **config) { 
  free((*config)->server);
  free((*config));
}

void ts_args_parse(int argc, char **argv, ts_args **ts_args) {

  int index;
  int c;

  opterr = 0;
  
  while ((c = getopt (argc, argv, "h:p:f:c:l:n:")) != -1) {
    switch (c)
    {
      case 'h':
        (*ts_args)->server->host = optarg;
        break;
      case 'p':
        (*ts_args)->server->port = atoi(optarg);
        break;
      case 'f':
        (*ts_args)->nc_conf_file = optarg;
        break;
      case 'c':
        (*ts_args)->nc_service_name = optarg;
        break;
      case 'l':
        (*ts_args)->nc_log_file = optarg;
        break;
      case 'n':
        (*ts_args)->nc_channel_name = optarg;
        break;
      case '?':
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return;
      default:
        abort();
    }
  }

  if((*ts_args)->server->host == NULL) {
    fprintf(stderr, "-h (nc_host) flag is required\n");
    exit(1);
  }
  else if((*ts_args)->server->port == 0) {
    fprintf(stderr, "-p (nc_port) flag is required\n");
    exit(1);
  }
  else if((*ts_args)->nc_conf_file == NULL) {
    fprintf(stderr, "-f (nc_conf_file) flag is required\n");
    exit(1);
  }
  else if((*ts_args)->nc_service_name == NULL) {
    fprintf(stderr, "-c (nc_service_name) flag is required\n");
    exit(1);
  }
  else {
    syslog(LOG_INFO, "TS ARGS: \n - ip: %s \n - port: %d\n - twemproxy_config_path: %s\n - twemproxy_service_name: %s\
     \n - log_file_location: %s\n - sentinel channel name: %s\n", 
     (*ts_args)->server->host, 
     (*ts_args)->server->port, 
     (*ts_args)->nc_conf_file, 
     (*ts_args)->nc_service_name,
     (*ts_args)->nc_log_file,
     (*ts_args)->nc_channel_name
    );
  }
}
