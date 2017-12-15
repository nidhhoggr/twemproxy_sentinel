#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include <ctype.h>

#include "ts_server.h"
#include "ts_sentinel.h"

redisContext* ts_sentinel_connect(ts_server **server) {
  
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds

  redis_ctx = redisConnectWithTimeout((*server)->host, (*server)->port, timeout);

  if (redis_ctx == NULL || redis_ctx->err) {
    syslog(LOG_CRIT, "Unable to connect to redis sentinel");
    exit(1);
  }
  
  return redis_ctx;
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
      syslog(LOG_INFO, "master (%d): %s:%hu\n", j, server->host, server->port);
    }
  }
  //freeReplyObject(reply);
  return servers;
}

void ts_sentinel_publish_message(redisAsyncContext *c, void *reply, void *privdata) {
  
  redisReply *r = reply;
  int j;

  if (reply == NULL) { return; }
  
  ts_args *tsArgs = malloc(sizeof(ts_args));
  
  tsArgs = (ts_args *)privdata;

  if (r->type == REDIS_REPLY_ARRAY) {

    for (j = 0; j < r->elements; j++) {
      syslog(LOG_INFO, "%u) %s\n", j, r->element[j]->str);
    }

    if (r->elements > 2 && !strcmp(r->element[0]->str, "message") 
      && !strcmp(r->element[1]->str, tsArgs->nc_channel_name)) {
      ts_master_promotion *mProm = ts_sentinel_parse_master_promotion(r->element[2]->str);
      ts_nc_update_masters_and_restart(&tsArgs);
    }
  }
}

void ts_sentinel_disconnect(const redisAsyncContext *c, int status) {

  int retries_remaining = 5;

  int reconnect;

  syslog(LOG_CRIT, "The connection to the sentinel was closed");

  while(retries_remaining-- > 0) {
    
    sleep(20);
    
    syslog(LOG_CRIT, "Attempting to recconect to redis sentinel with %d retries remaining", retries_remaining);
     
    reconnect = redisReconnect(redis_ctx);

    if(reconnect > -1) {
      syslog(LOG_INFO, "Was successfully able to reconnect");
      break;
    }
  }

  if(retries_remaining < 0) exit(-1);
}

int ts_sentinel_subscribe(ts_args **tsArgs) {

  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  redisAsyncContext *c = redisAsyncConnect((*tsArgs)->server->host, (*tsArgs)->server->port);
  if (c->err) {
    syslog(LOG_CRIT, "error: %s\n", c->errstr);
    return 1;
  }

  redisEnableKeepAlive(&c->c);

  redisLibeventAttach(c, base);
  char subscribeCmd[72];
  sprintf(subscribeCmd,"SUBSCRIBE %s", (*tsArgs)->nc_channel_name);
  redisAsyncCommand(c, ts_sentinel_publish_message, (*tsArgs), subscribeCmd);
  redisAsyncSetDisconnectCallback(c, ts_sentinel_disconnect);
  syslog(LOG_INFO, "twemproxy sentinel listenting to sentinel on channel: %s\n", subscribeCmd);
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

void ts_master_promotion_free(ts_master_promotion **mProm) {
  free((*mProm)->old_master); 
  free((*mProm)->new_master); 
  free((*mProm));
}

ts_master_promotion *ts_sentinel_parse_master_promotion(char *master_promotion_msg) {

  ts_master_promotion *master_promotion = ts_master_promotion_init();
  master_promotion->old_master->name = strtok(master_promotion_msg, " ");
  master_promotion->old_master->host = strtok(NULL, " ");
  master_promotion->old_master->port = atoi(strtok(NULL, " "));
  master_promotion->new_master->name = master_promotion->old_master->name;
  master_promotion->new_master->host = strtok(NULL, " ");
  master_promotion->new_master->port = atoi(strtok(NULL, " "));
  strtok(NULL, " ");

  syslog(LOG_NOTICE, "Old Master: %s:%hu:1 %s\nNew Master: %s:%hu:1 %s\n", 
    master_promotion->old_master->host,
    master_promotion->old_master->port,
    master_promotion->old_master->name, 
    master_promotion->new_master->host, 
    master_promotion->new_master->port,
    master_promotion->old_master->name);

  return master_promotion;
}
