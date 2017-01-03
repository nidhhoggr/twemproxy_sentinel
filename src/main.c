#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <yaml.h>

#include "logging.h"

struct tsConf {
  char *nc_host;
  uint16_t nc_port;
  char *nc_conf_file;
  char *nc_service_name;
  char *nc_log_file;
};

void main(int argc, char **argv) {

  parseyaml();

  return;

  struct tsConf ts_conf = {};

  parseConfig(argc, argv, &ts_conf);

  int restart_status = 0;


  //restart_status = execute_service_restart(ts_conf.nc_service_name);

  //if(restart_status == 0) {
  //  printf("restart failed");
  //}
}

void parseConfig(int argc, char **argv, struct tsConf *ts_conf) {

  char logging[200];

  int index;
  int c;

  opterr = 0;
  
  while ((c = getopt (argc, argv, "h:p:f:c:l:")) != -1) {
    switch (c)
    {
      case 'h':
        ts_conf->nc_host = optarg;
        break;
      case 'p':
        ts_conf->nc_port = atoi(optarg);
        break;
      case 'f':
        ts_conf->nc_conf_file = optarg;
        break;
      case 'c':
        ts_conf->nc_service_name = optarg;
        break;
      case 'l':
        ts_conf->nc_log_file = optarg;
        break;
      case '?':
        fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        return;
      default:
        abort();
    }
  }

  if(ts_conf->nc_host == NULL) {
    fprintf(stderr, "-h (nc_host) flag is required\n");
  }
  else if(ts_conf->nc_port == 0) {
    fprintf(stderr, "-p (nc_port) flag is required\n");
  }
  else if(ts_conf->nc_conf_file == NULL) {
    fprintf(stderr, "-f (nc_conf_file) flag is required\n");
  }
  else if(ts_conf->nc_service_name == NULL) {
    fprintf(stderr, "-c (nc_service_name) flag is required\n");
  }
  else {
    sprintf(logging, "ip: %s \nport: %d\ntwemproxy_config_path: %s\ntwemproxy_service_name: %s\nlog_file_location: %s\n", 
     ts_conf->nc_host, ts_conf->nc_port, ts_conf->nc_conf_file, ts_conf->nc_service_name, ts_conf->nc_log_file);
    printf("%s\n", logging);
    log_info(logging);
  }
}


int execute_service_restart(char *service_name) {
  pid_t pid;
  int status;
  if ((pid = fork()) == 0) {
    /* the child process */
    execlp("service", "service", service_name, "restart", NULL);
    /* if execl() was successful, this won't be reached */
    _exit(0);
  }

  if ((pid > 0) && (waitpid(pid, &status, 0) > 0) && (WIFEXITED(status) && !WEXITSTATUS(status))) {
    return 1;
  }

  return 0;
}

int parseyaml(void)
{
  FILE *fh = fopen("conf/nutcracker.yml", "r");
  yaml_parser_t parser;
  yaml_token_t  token;   /* new variable */

  /* Initialize parser */
  if(!yaml_parser_initialize(&parser))
    fputs("Failed to initialize parser!\n", stderr);
  if(fh == NULL)
    fputs("Failed to open file!\n", stderr);

  /* Set input file */
  yaml_parser_set_input_file(&parser, fh);

  /* BEGIN new code */
  do {
    yaml_parser_scan(&parser, &token);
    switch(token.type)
    {
    /* Stream start/end */
    case YAML_STREAM_START_TOKEN: puts("STREAM START"); break;
    case YAML_STREAM_END_TOKEN:   puts("STREAM END");   break;
    /* Token types (read before actual token) */
    case YAML_KEY_TOKEN:   printf("(Key token)   "); break;
    case YAML_VALUE_TOKEN: printf("(Value token) "); break;
    /* Block delimeters */
    case YAML_BLOCK_SEQUENCE_START_TOKEN: puts("<b>Start Block (Sequence)</b>"); break;
    case YAML_BLOCK_ENTRY_TOKEN:          puts("<b>Start Block (Entry)</b>");    break;
    case YAML_BLOCK_END_TOKEN:            puts("<b>End block</b>");              break;
    /* Data */
    case YAML_BLOCK_MAPPING_START_TOKEN:  puts("[Block mapping]");            break;
    case YAML_SCALAR_TOKEN:  printf("scalar %s \n", token.data.scalar.value); break;
    /* Others */
    default:
      printf("Got token of type %d\n", token.type);
    }
    if(token.type != YAML_STREAM_END_TOKEN)
      yaml_token_delete(&token);
  } while(token.type != YAML_STREAM_END_TOKEN);
  yaml_token_delete(&token);
  /* END new code */

  /* Cleanup */
  yaml_parser_delete(&parser);
  fclose(fh);
  return 0;
}
