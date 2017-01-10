#include <stdint.h>

#ifndef _TS_SERVER_H
#define _TS_SERVER_H

struct ts_server_struct {
  uint16_t port;
  char *host;
  char *name;
};

typedef struct ts_server_struct ts_server;

struct ts_servers_struct {
  ts_server *server;
  struct ts_servers_struct *next;
};

typedef struct ts_servers_struct ts_servers;

ts_servers* ts_create_servers(ts_server **ts_server);
ts_servers* ts_add_server(ts_servers **ts_servers, ts_server **ts_server);
void ts_delete_servers(ts_servers **ts_servers);

#endif
