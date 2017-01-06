#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <yaml.h>

#include "ts_server.h"
#include "ts_sentinel.h"
#include "ts_args.h"
#include "ts_nc_config.h"

ts_nc_config_parser_events * ts_nc_config_parser_events_init(yaml_event_t *event) {
  ts_nc_config_parser_events *newEvents = malloc(sizeof(ts_nc_config_parser_events));
  if(NULL != newEvents) {
    newEvents->event = malloc(sizeof(yaml_event_t));
    printf("initialized with event type: %d", event->type);
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

  /* suppose first = {1, 2, 3}, rest = {2, 3} */
  first = *head_ref;  
  rest  = first->next;

  /* List has only one node */
  if (rest == NULL)
    return;   

  /* reverse the rest list and put the first element at the end */
  ts_nc_config_parser_events_reverse(&rest);
  first->next->next  = first;  

  /* tricky step -- see the diagram */
  first->next  = NULL;          

  /* fix the head pointer */
  *head_ref = rest;
}

int ts_nc_config_update(ts_args **tsArgs, ts_servers **servers)
{
  ts_nc_config_parser_events *events = ts_nc_config_parse(tsArgs, servers);

  ts_nc_config_parser_events_reverse(&events); 

  /*
  while(events != NULL) {
    printf("%d\n", events->event->type);
    
    if(events->next && events->next->event) {
      events = events->next;
    }
    else {
      events = NULL;
    }
  }
  */

  ts_nc_config_emit(&events);

  ts_nc_config_parser_events_free(&events);
}

ts_nc_config_parser_events * ts_nc_config_parse(ts_args **tsArgs, ts_servers **servers) {
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

  FILE *ifp;

  ifp = fopen((*tsArgs)->nc_conf_file, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Can't open input file\n");
    exit(1);
  }

  yaml_parser_set_input_file(&parser, ifp);

  int beginServerNodes = 0;

  ts_servers *serverIt = calloc(10, sizeof(ts_server));
  
  /* The main loop. */

  int parsedFirst = 0;

  while (!done)
  {
    yaml_event_t *event = malloc(sizeof(yaml_event_t));
    /* Get the next token. */
    
    if (yaml_parser_parse(&parser, event)) {
      if(!parsedFirst) {
        printf("events are null");
        if (event->type == YAML_STREAM_START_EVENT)
          printf("+STR\n");
        events = ts_nc_config_parser_events_init(event);
        parsedFirst = 1;
      }
      else {
        events = ts_nc_config_parser_events_push(&events, event);
      }
    }
    else {
      handle_parser_error(&parser);
    }

    printf(" -- \n");

    /* Check if this is the stream end. */

    if(beginServerNodes && event->type == 8) {
      beginServerNodes = 0;
    }

    if (event->type == YAML_SCALAR_EVENT) {

      if(beginServerNodes == 1) {

        serverIt = (*servers);
        while(serverIt != NULL) {
          printf("nextServer: %s\n", serverIt->server->host);
          if(serverIt->server->host != NULL) {
            serverIt = serverIt->next;
          }
        }

        printf("%s\n", event->data.scalar.value);
        //const char* newHost = "10.132.16.48:6379:1 redis-001";
        //event.data.scalar.value = (yaml_char_t*)strdup(newHost); 
        //event.data.scalar.length = strlen(newHost);
      }
      if(strcmp("servers",event->data.scalar.value) == 0) {
        beginServerNodes = 1;
      }
    }

    if (event->type == YAML_STREAM_END_EVENT) {
      done = 1;
    } 
  }

  yaml_parser_delete(&parser);

  return events;
}

void handle_parser_error(yaml_parser_t *parser) {
  /* Display a parser error message. */
  switch (parser->error)
  {
    case YAML_MEMORY_ERROR:
      fprintf(stderr, "Memory error: Not enough memory for parsing\n");
      break;

    case YAML_READER_ERROR:
      if (parser->problem_value != -1) {
        fprintf(stderr, "Reader error: %s: #%X at %d\n", parser->problem,
            parser->problem_value, parser->problem_offset);
      }
      else {
        fprintf(stderr, "Reader error: %s at %d\n", parser->problem,
            parser->problem_offset);
      }
      break;

    case YAML_SCANNER_ERROR:
      if (parser->context) {
        fprintf(stderr, "Scanner error: %s at line %d, column %d\n"
            "%s at line %d, column %d\n", parser->context,
            parser->context_mark.line+1, parser->context_mark.column+1,
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      else {
        fprintf(stderr, "Scanner error: %s at line %d, column %d\n",
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      break;

    case YAML_PARSER_ERROR:
      if (parser->context) {
        fprintf(stderr, "Parser error: %s at line %d, column %d\n"
            "%s at line %d, column %d\n", parser->context,
            parser->context_mark.line+1, parser->context_mark.column+1,
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      else {
        fprintf(stderr, "Parser error: %s at line %d, column %d\n",
            parser->problem, parser->problem_mark.line+1,
            parser->problem_mark.column+1);
      }
      break;

    default:
      /* Couldn't happen. */
      fprintf(stderr, "Internal error\n");
      break;
  }

  yaml_parser_delete(parser);
}

int ts_nc_config_emit(ts_nc_config_parser_events **events) {

  yaml_emitter_t emitter;
  memset(&emitter, 0, sizeof(emitter));

  if (!yaml_emitter_initialize(&emitter)){
    handle_emitter_error(&emitter);
    return 1;
  }

  /* Set the parser parameters. */

  FILE *ofp;

  ofp = fopen("conf/output.yml"/*tsArgs->nc_conf_file*/, "w");

  if (ofp == NULL) {
    fprintf(stderr, "Can't open output file\n");
    exit(1);
  }

  yaml_emitter_set_output_file(&emitter, ofp);

  while((*events) != NULL) {

    if ((*events)->event->type == YAML_SCALAR_EVENT) {

        printf("emitter scalar output: %s\n", (*events)->event->data.scalar.value);
        //const char* newHost = "10.132.16.48:6379:1 redis-001";
        //event.data.scalar.value = (yaml_char_t*)strdup(newHost); 
        //event.data.scalar.length = strlen(newHost);
    }

    if (!yaml_emitter_emit(&emitter, (*events)->event)) {
      handle_emitter_error(&emitter);
      return 1;
    }
    if((*events)) {
      (*events) = (*events)->next;
    }
  }

  yaml_emitter_delete(&emitter);

  return 0;
}

void handle_emitter_error(yaml_emitter_t *emitter) {
  /* Display an emitter error message. */

  switch (emitter->error)
  {
    case YAML_MEMORY_ERROR:
      fprintf(stderr, "Memory error: Not enough memory for emitting\n");
      break;

    case YAML_WRITER_ERROR:
      fprintf(stderr, "Writer error: %s\n", emitter->problem);
      break;

    case YAML_EMITTER_ERROR:
      fprintf(stderr, "Emitter error: %s\n", emitter->problem);
      break;

    default:
      /* Couldn't happen. */
      fprintf(stderr, "Internal error\n");
      break;
  }

  yaml_emitter_delete(emitter);
}
