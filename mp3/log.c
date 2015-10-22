/** @file log.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

/**
 * Initializes the log.
 *
 * You may assuem that:
 * - This function will only be called once per instance of log_t.
 * - This function will be the first function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @returns
 *   The initialized log structure.
 */
void log_init(log_t *l) {
	l->ptr = NULL;
	l->length = 0;
}

/**
 * Frees all internal memory associated with the log.
 *
 * You may assume that:
 * - This function will be called once per instance of log_t.
 * - This funciton will be the last function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure to be destoryed.
 */
void log_destroy(log_t* l) {
	int i;
	for(i = 0; i < l->length; i++) {
		free(l->ptr[i]);
	}
	free(l->ptr);
}

/**
 * Push an item to the log stack.
 *
 *
 * You may assume that:
* - All pointers will be valid, non-NULL pointer.
*
 * @param l
 *    Pointer to the log data structure.
 * @param item
 *    Pointer to a string to be added to the log.
 */
void log_push(log_t* l, const char *item) {
	l->length += 1;
	char** new_ptr = NULL;
	new_ptr = (char**)realloc(l->ptr, l->length * sizeof(char *));
	l->ptr = new_ptr;
	l->ptr[l->length - 1] = item;
}


/**
 * Preforms a newest-to-oldest search of log entries for an entry matching a
 * given prefix.
 *
 * This search starts with the most recent entry in the log and
 * compares each entry to determine if the query is a prefix of the log entry.
 * Upon reaching a match, a pointer to that element is returned.  If no match
 * is found, a NULL pointer is returned.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param prefix
 *    The prefix to test each entry in the log for a match.
 *
 * @returns
 *    The newest entry in the log whose string matches the specified prefix.
 *    If no strings has the specified prefix, NULL is returned.
 */
char *log_search(log_t* l, const char *prefix) {
	int backtrack = l->length - 1;
	while(backtrack != -1) {
		if(strncmp(l->ptr[backtrack], prefix, strlen(prefix)) == 0) {
			return l->ptr[backtrack];
		}
		backtrack--; 
	}
	return NULL;
}

void log_print(log_t* l) {
	int i;
	for(i = 0; i < l->length; i++) {
		printf("%s\n", l->ptr[i]);
	}
}
