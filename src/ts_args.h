#ifndef _TS_ARGS_H
#define _TS_ARGS_H

struct tsArgs {
  ts_server *server;
  char *nc_conf_file;
  char *nc_service_name;
  char *nc_log_file;
  char *nc_channel_name;
};

typedef struct tsArgs ts_args;

ts_args* ts_args_init(void);
void ts_args_free(ts_args **config);
void ts_args_parse(int argc, char **argv, ts_args **ts_args);

#endif
