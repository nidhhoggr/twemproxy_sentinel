#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ts_server.h"

ts_servers* ts_create_servers(ts_server **server) {
  ts_servers  *newServers = malloc(sizeof(ts_servers));
  if (NULL != newServers) {
    newServers->server = (*server);
    newServers->next = NULL;
  }
  return newServers;
}

ts_servers* ts_add_server(ts_servers **servers, ts_server **server) {
  ts_servers *newServers = ts_create_servers(server);
  if (NULL != newServers) {
    newServers->next = *servers;
  }
  return newServers;
}

void ts_delete_servers(ts_servers **servers) {
  if(NULL == (*servers)) {
    return;
  }
  else if (NULL != (*servers)->next) {
    ts_delete_servers(servers);
  }
  free(servers);
}
