#ifndef _TS_SERVER_H
#define _TS_SERVER_H

struct tsServer {
  char *port;
  char *host;
  char *name;
};

typedef struct tsServer ts_server;

struct tsServers {
  ts_server *server;
  struct tsServers *next;
};

typedef struct tsServers ts_servers;

ts_servers* ts_create_servers(ts_server *ts_server);
ts_servers* ts_add_server(ts_servers *ts_servers, ts_server *ts_server);
void ts_delete_servers(ts_servers *ts_servers);
const char * ts_set_server_fqn(ts_server *server);

#endif
