#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include "ts_server.h"
#include "ts_sentinel.h"


redisContext* ts_sentinel_connect(ts_server *server) {
  
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds

  redisContext *c;
  c = redisConnectWithTimeout(server->host, server->port, timeout);

  if (c == NULL || c->err) {
    ts_sentinel_disconnect(c);
    exit(1);
  }
  
  return c;
}

void ts_sentinel_disconnect(redisContext *c) {
  if (c) {
    printf("Connection error: %s\n", c->errstr);
    redisFree(c);
  } else {
    printf("Connection error: can't allocate redis context\n");
  }
}

int ts_sentinel_set_masters(redisContext *c, ts_servers *servers) {
  redisReply *reply;
  int j;

  reply = redisCommand(c,"SENTINEL %s","masters");
  
  if (reply->type == REDIS_REPLY_ARRAY) {
    for (j = 0; j < reply->elements; j++) {
      ts_server server;
      server.name = reply->element[j]->element[1]->str;
      server.host = reply->element[j]->element[3]->str;
      server.port = atoi(reply->element[j]->element[5]->str);
      ts_server *sPtr = &server;
      servers = ts_add_server(servers, sPtr);
      const char *server_fqn = ts_set_server_fqn(sPtr);
      printf("master (%d): %s\n", j, server_fqn);
    }
  }

  freeReplyObject(reply);

  return 0;
}

void ts_sentinel_publish_message(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;
  int j;

  if (reply == NULL) return;

  if (r->type == REDIS_REPLY_ARRAY) {
    for (j = 0; j < r->elements; j++) {
      printf("%u) %s\n", j, r->element[j]->str);
    }
  }
}


int ts_sentinel_subscribe(ts_server *server) {

  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  redisAsyncContext *c = redisAsyncConnect(server->host, server->port);
  if (c->err) {
    printf("error: %s\n", c->errstr);
    return 1;
  }

  redisLibeventAttach(c, base);
  redisAsyncCommand(c, ts_sentinel_publish_message, NULL, "SUBSCRIBE +switch-master");
  event_base_dispatch(base);

  return 0;
}
