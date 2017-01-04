#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ts_servers {
  struct ts_server *server;
  struct ts_servers *next;
} ts_servers;

typedef struct ts_server {
  char *port;
  char *host;
  char *name;
} ts_server;

ts_servers* ts_create_servers(ts_server *server) {
  ts_servers* newServers = malloc(sizeof(ts_servers));
  if (NULL != newServers) {
    newServers->server = server;
    newServers->next = NULL;
  }
  return newServers;
}

ts_servers* ts_add_server(ts_servers *servers, ts_server *server) {
  ts_servers* newServers = ts_create_servers(server);
  if (NULL != newServers) {
    newServers->next = servers;
  }
  return newServers;
}

void ts_delete_servers(ts_servers *servers) {
  if (NULL != servers->next) {
    ts_delete_servers(servers->next);
  }
  free(servers);
}

const char * ts_set_server_fqn(ts_server *server) {
  char *server_fqn;
  server_fqn = malloc(strlen(server->name) + (5 * sizeof(char)));
  strcpy(server_fqn, server->host);
  strcat(server_fqn, ":");
  strcat(server_fqn, server->port);
}
