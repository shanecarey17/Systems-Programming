/** @file log.h */

#ifndef __LOG_H_
#define __LOG_H_

typedef struct _log_t {
	char **ptr;
	int length;    
} log_t;

void log_init(log_t *l);
void log_destroy(log_t* l);
void log_push(log_t* l, const char *item);
char *log_search(log_t* l, const char *prefix);
void log_print(log_t* l);

#endif
