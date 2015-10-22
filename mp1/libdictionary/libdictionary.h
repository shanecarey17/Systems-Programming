/** @file libdictioanry.h */

/*
 * Machine Problem #1
 * CS 241
 */

#ifndef LIBDICTIONARY_H__
#define LIBDICTIONARY_H__

extern const int NO_KEY_EXISTS;
extern const int KEY_EXISTS;
extern const int ILLEGAL_FORMAT;

typedef struct dictionary_entry_t
{
	const char *key;
	const char *value;
	struct dictionary_entry_t *next;
} dictionary_entry_t;

typedef struct dictionary_t
{
	struct dictionary_entry_t *head;
} dictionary_t;

void dictionary_print(dictionary_t *d);
void dictionary_init(dictionary_t *d);
int dictionary_add(dictionary_t *d, const char *key, const char *value);
int dictionary_parse(dictionary_t *d, char *key_value);
const char *dictionary_get(dictionary_t *d, const char *key);
int dictionary_remove(dictionary_t *d, const char *key);
void dictionary_destroy(dictionary_t *d);

#endif
