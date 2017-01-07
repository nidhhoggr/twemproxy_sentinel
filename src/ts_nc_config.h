#ifndef _TS_NC_CONFIG_H
#define _TS_NC_CONFIG_H

#include <yaml.h>

#include "ts_server.h"
#include "ts_args.h"
#include "ts_nc_config.h"

struct ts_nc_config_parser_events_struct {
  yaml_event_t *event;
  struct ts_nc_config_parser_events_struct *next;
};

typedef struct ts_nc_config_parser_events_struct ts_nc_config_parser_events;

int ts_nc_config_update(ts_args **tsArgs, ts_servers **servers);
ts_nc_config_parser_events * ts_nc_config_parse(ts_args **tsArgs, ts_servers **servers);
void handle_parser_error(yaml_parser_t *parser);
int ts_nc_config_emit(ts_args **tsArgs, ts_nc_config_parser_events **events);
void handle_emitter_error(yaml_emitter_t *emitter);
ts_nc_config_parser_events * ts_nc_config_parser_events_init(yaml_event_t *event);
ts_nc_config_parser_events * ts_nc_config_parser_events_push(ts_nc_config_parser_events **events, yaml_event_t *event);
void ts_nc_config_parser_events_free(ts_nc_config_parser_events **events);
void ts_nc_config_parser_events_reverse(ts_nc_config_parser_events **head_ref);

#endif
