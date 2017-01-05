#ifndef _TS_SENTINEL_H
#define _TS_SENTINEL_H

#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

struct ts_master_promotion_struct {
  ts_server *old_master;
  ts_server *new_master;
};

typedef struct  ts_master_promotion_struct ts_master_promotion;

redisContext* ts_sentinel_connect(ts_server *server);
void ts_sentinel_disconnect(redisContext *c);
int ts_sentinel_set_masters(redisContext *c, ts_servers *servers);
void ts_sentinel_publish_message(redisAsyncContext *c, void *reply, void *privdata);
int ts_sentinel_subscribe(ts_server *server);
ts_master_promotion *ts_master_promotion_init(void);
ts_master_promotion *ts_sentinel_parse_master_promotion(char *master_promotion_msg);

#endif
