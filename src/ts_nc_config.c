#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>
#include <syslog.h>

#include "ts_server.h"
#include "ts_sentinel.h"
#include "ts_args.h"
#include "ts_nc_config.h"

ts_nc_config_parser_events * ts_nc_config_parser_events_init(yaml_event_t *event) {
  ts_nc_config_parser_events *newEvents = malloc(sizeof(ts_nc_config_parser_events));
  if(NULL != newEvents) {
    newEvents->event = event;
    newEvents->next = NULL;
  }
  return newEvents;
}

ts_nc_config_parser_events * ts_nc_config_parser_events_push(
  ts_nc_config_parser_events **events, yaml_event_t *event) {
  ts_nc_config_parser_events *newEvents = ts_nc_config_parser_events_init(event);
  if(NULL != events) {
    newEvents->next = (*events);
  }
  return newEvents;
}

void ts_nc_config_parser_events_free(ts_nc_config_parser_events **events) {
  if(NULL == (*events)) {
    return;
  }
  else if(NULL != (*events)->next) {
    ts_nc_config_parser_events_free(events);
  }
  free((*events));
}

void ts_nc_config_parser_events_reverse(ts_nc_config_parser_events **head_ref) {
  
  ts_nc_config_parser_events *first;
  ts_nc_config_parser_events *rest;

  /* empty list */
  if (*head_ref == NULL)
    return;   

  first = *head_ref;  
  rest  = first->next;

  /* List has only one node */
  if (rest == NULL)
    return;   

  ts_nc_config_parser_events_reverse(&rest);
  
  first->next->next  = first;  

  first->next  = NULL;          

  *head_ref = rest;
}

int ts_nc_config_update(ts_args **tsArgs, ts_servers **servers)
{
  ts_nc_config_parser_events *events = ts_nc_config_parse(tsArgs, servers);

  ts_nc_config_parser_events_reverse(&events); 

  ts_nc_config_emit(tsArgs, &events);

  ts_nc_config_parser_events_free(&events);
}

ts_nc_config_parser_events * ts_nc_config_parse(ts_args **tsArgs, ts_servers **servers) {
  
  FILE *ifp;

  ifp = fopen((*tsArgs)->nc_conf_file, "r");

  if (ifp == NULL) {
    syslog(LOG_CRIT, "Can't open input file\n");
    exit(1);
  }


  int done = 0;

  yaml_parser_t parser;
  ts_nc_config_parser_events *events = {NULL};

  /* Clear the objects. */

  memset(&parser, 0, sizeof(parser));

  /* Initialize the parser and emitter objects. */

  if (!yaml_parser_initialize(&parser)) {
    handle_parser_error(&parser);
    return events;
  }
  /* Set the parser parameters. */


  yaml_parser_set_input_file(&parser, ifp);

  int beginServerNodes = 0;

  ts_servers *serverIt = malloc(sizeof(ts_servers));
  
  int parsedFirst = 0;

  char *event_server_uri; 
  ts_server *event_server = malloc(sizeof(ts_server));

  while (!done)
  {
    yaml_event_t *event = malloc(sizeof(yaml_event_t));
    
    if (yaml_parser_parse(&parser, event)) {
      if(!parsedFirst) {
        events = ts_nc_config_parser_events_init(event);
        parsedFirst = 1;
      }
      else {
        events = ts_nc_config_parser_events_push(&events, event);
      }
    }
    else {
      handle_parser_error(&parser);
      ts_nc_config_parser_events_free(&events);
      free(event_server);
      free(serverIt);
      exit(0);
    }

    if(beginServerNodes && event->type == YAML_BLOCK_MAPPING_START_TOKEN) {
      beginServerNodes = 0;
    }

    if (event->type == YAML_SCALAR_EVENT) {

      if(beginServerNodes == 1) {

        event_server_uri = strtok(event->data.scalar.value," ");
        event_server->name = strtok(NULL, " ");
        event_server->host = strtok(event_server_uri, ":");
        event_server->port = atoi(strtok(NULL, ":"));

        serverIt = (*servers);
        while(serverIt != NULL) {
         
          //names match so lets check URI
          if(strcmp(serverIt->server->name, event_server->name) == 0) {
            syslog(LOG_INFO, "MATCH: master name: %s, config name: %s\n", 
             serverIt->server->name,
             event_server->name);
             
            char promotedServer[36];
            
            //has matching URI?
            if(strcmp(serverIt->server->host, event_server->host) != 0 ||
              serverIt->server->port != event_server->port) {
              syslog(LOG_NOTICE, "DOESNT MATCH: master uri: %s:%hu,  config uri %s:%hu\n",
                serverIt->server->host,
                serverIt->server->port,
                event_server->host,
                event_server->port);

              sprintf(promotedServer, "%s:%hu:1 %s",
                serverIt->server->host,
                serverIt->server->port,
                serverIt->server->name
              );
            
              syslog(LOG_NOTICE, "promoting master to %s\n", promotedServer);

              event->data.scalar.value = (yaml_char_t*)strdup(promotedServer); 
              event->data.scalar.length = strlen(promotedServer);
            }
            else {
              sprintf(promotedServer, "%s:%hu:1 %s",
                  serverIt->server->host,
                  serverIt->server->port,
                  serverIt->server->name
                  );

              event->data.scalar.value = (yaml_char_t*)strdup(promotedServer); 
              event->data.scalar.length = strlen(promotedServer);
            }
          }

          if(serverIt->server->host != NULL) {
            serverIt = serverIt->next;
          }
        }

      }
      if(strcmp("servers",event->data.scalar.value) == 0) {
        beginServerNodes = 1;
      }
    }
    else if (event->type == YAML_STREAM_END_EVENT) {
      done = 1;
    } 
  }

  yaml_parser_delete(&parser);
  free(event_server);
  ts_delete_servers(&serverIt);
  fclose(ifp);

  return events;
}

void handle_parser_error(yaml_parser_t *parser) {
  /* Display a parser error message. */
  switch (parser->error)
  {
    case YAML_MEMORY_ERROR:
      syslog(LOG_CRIT, "Memory error: Not enough memory for parsing\n");
      break;

    case YAML_READER_ERROR:
      if (parser->problem_value != -1) {
        syslog(LOG_CRIT, "Reader error: %s: #%X at %d\n", parser->problem,
            parser->problem_value, parser->problem_offset);
      }
      else {
        syslog(LOG_CRIT, "Reader error: %s at %d\n", parser->problem,
            parser->problem_offset);
      }
      break;

    case YAML_SCANNER_ERROR:
      if (parser->context) {
        syslog(LOG_CRIT, "Scanner error: %s at line %d, column %d\n"
            "%s at line %d, column %d\n", parser->context,
            parser->context_mark.line+1, parser->context_mark.column+1,
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      else {
        syslog(LOG_CRIT, "Scanner error: %s at line %d, column %d\n",
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      break;

    case YAML_PARSER_ERROR:
      if (parser->context) {
        syslog(LOG_CRIT, "Parser error: %s at line %d, column %d\n"
            "%s at line %d, column %d\n", parser->context,
            parser->context_mark.line+1, parser->context_mark.column+1,
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      else {
        syslog(LOG_CRIT, "Parser error: %s at line %d, column %d\n",
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      break;

    default:
      /* Couldn't happen. */
      syslog(LOG_CRIT, "Internal error\n");
      break;
  }

  yaml_parser_delete(parser);
}

int ts_nc_config_emit(ts_args **tsArgs, ts_nc_config_parser_events **events) {

  FILE *ofp;

  ofp = fopen((*tsArgs)->nc_conf_file, "w");

  if (ofp == NULL) {
    syslog(LOG_CRIT, "Can't open output file\n");
    exit(1);
  }

  yaml_emitter_t emitter;

  memset(&emitter, 0, sizeof(emitter));

  if (!yaml_emitter_initialize(&emitter)){
    handle_emitter_error(&emitter);
    return 1;
  }

  yaml_emitter_set_output_file(&emitter, ofp);

  while((*events) != NULL) {

    if (!yaml_emitter_emit(&emitter, (*events)->event)) {
      handle_emitter_error(&emitter);
      return 1;
    }
   
    free((*events)->event);
    (*events) = (*events)->next;
  }

  
  yaml_emitter_delete(&emitter);
  fclose(ofp);

  return 0;
}

void handle_emitter_error(yaml_emitter_t *emitter) {
  /* Display an emitter error message. */

  switch (emitter->error)
  {
    case YAML_MEMORY_ERROR:
      syslog(LOG_CRIT, "Memory error: Not enough memory for emitting\n");
      break;

    case YAML_WRITER_ERROR:
      syslog(LOG_CRIT, "Writer error: %s\n", emitter->problem);
      break;

    case YAML_EMITTER_ERROR:
      syslog(LOG_CRIT, "Emitter error: %s\n", emitter->problem);
      break;

    default:
      /* Couldn't happen. */
      syslog(LOG_CRIT, "Internal error\n");
      break;
  }

  yaml_emitter_delete(emitter);
}
