#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include "ts_server.h"
#include "ts_sentinel.h"


redisContext* ts_sentinel_connect(ts_server **server) {
  
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds

  redisContext *c;
  c = redisConnectWithTimeout((*server)->host, (*server)->port, timeout);

  if (c == NULL || c->err) {
    ts_sentinel_disconnect(&c);
    exit(1);
  }
  
  return c;
}

void ts_sentinel_disconnect(redisContext **c) {
  if (!(*c)) {
    printf("Connection error: %s\n", (*c)->errstr);
  }
}

ts_servers * ts_sentinel_get_masters(redisContext **c) {
  redisReply *reply;
  int j;

  reply = redisCommand((*c),"SENTINEL %s","masters");

  ts_servers *servers = {NULL};

  if (reply->type == REDIS_REPLY_ARRAY) {

    for (j = 0; j < reply->elements; j++) {
      ts_server *server = calloc(1, sizeof(ts_server));
      
      /*
      memcpy(&server->name, &reply->element[j]->element[1]->str, 
        sizeof(reply->element[j]->element[1]->str));
      memcpy(&server->host, &reply->element[j]->element[3]->str,
        sizeof(reply->element[j]->element[3]->str));
      memcpy(&server->port, &reply->element[j]->element[5]->str, 2);
      */

      server->name = reply->element[j]->element[1]->str;
      server->host = reply->element[j]->element[3]->str;
      server->port = atoi(reply->element[j]->element[5]->str);

      if(servers == NULL) {
        servers = ts_create_servers(&server);
      }
      else {
        servers = ts_add_server(&servers, &server);
      }
      const char *server_fqn = ts_set_server_fqn(&server);
      printf("master (%d): %s\n", j, server_fqn);
    }
  }
  //freeReplyObject(reply);
  return servers;
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

ts_master_promotion *ts_master_promotion_init(void) {
  ts_master_promotion *mProm = calloc(1, sizeof(ts_master_promotion));
  ts_server *oMaster = calloc(1, sizeof(ts_server));
  ts_server *nMaster = calloc(1, sizeof(ts_server));
  mProm->old_master = oMaster;
  mProm->new_master = nMaster;
  return mProm;
}

ts_master_promotion *ts_sentinel_parse_master_promotion(char *master_promotion_msg) {
  
  printf ("Splitting string \"%s\" into tokens:\n", master_promotion_msg);
  ts_master_promotion *master_promotion = ts_master_promotion_init();
  
  master_promotion->old_master->name = strtok(master_promotion_msg, " ");
  master_promotion->old_master->host = strtok(NULL, " ");
  master_promotion->old_master->port = atoi(strtok(NULL, " "));
  master_promotion->new_master->name = master_promotion->old_master->name;
  master_promotion->new_master->host = strtok(NULL, " ");
  master_promotion->new_master->port = atoi(strtok(NULL, " "));
  strtok(NULL, " ");

  printf("%s - %s - %hu - %s - %hu", 
    master_promotion->old_master->name, 
    master_promotion->old_master->host,
    master_promotion->old_master->port,
    master_promotion->new_master->host, 
    master_promotion->new_master->port);

  return master_promotion;
}
