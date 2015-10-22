/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/epoll.h>

#include "libmapreduce.h"
#include "libds/libds.h"


static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */


/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{
	// Grab the value from dictionary and replace
	const char *temp = datastore_get(mr->dictionary, key, (long unsigned int *)&(mr->revision));
	if(temp != NULL) {
		// Value exists
		char *new = (char *)mr->reduce(temp, value);
		mr->revision = datastore_update(mr->dictionary, key, new, mr->revision);
		free(new);
	}
	// Value does not exist
	else mr->revision = datastore_put(mr->dictionary, key, value);

	// Free memory
	free((char *) key);
	free((char *) value);
	free((char *) temp);
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}
	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
	}

	return 1;
}


/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, void (*mymap)(int, const char *), const char *(*myreduce)(const char *, const char *))
{
	mr->map = mymap;
	mr->reduce = myreduce;
	mr->dictionary = malloc(sizeof(datastore_t));
	datastore_init(mr->dictionary);
	mr->data_sets = 0;
}

void *thread_reduce(void *arg) {
	
	// Pass in mapreduce structure and initialize buffers
	mapreduce_t *mr = (mapreduce_t *)arg;
	mr->buffer = malloc(sizeof(char *) * mr->data_sets);
	int i;
	for(i = 0; i < mr->data_sets; i++) {
		mr->buffer[i] = malloc(sizeof(char) * (BUFFER_SIZE + 1));
		mr->buffer[i][0] = '\0';
	}
	
	// Read in data via epoll_wait
	struct epoll_event ev;
	int done = 0;
	while(done < mr->data_sets) {
		epoll_wait(mr->epoll_fd, &ev, 1, -1);
		if(read_from_fd(ev.data.fd, mr->buffer[done], mr) == 0) {
			epoll_ctl(mr->epoll_fd, EPOLL_CTL_DEL, ev.data.fd, NULL);
			done++;
		}
	}
	
	// Return
	return NULL;
}


/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)
{
	// Establish number of data sets
	while(values[mr->data_sets]) {
		mr->data_sets++;
	}

	// Create pipe for each data set in pipes array
	mr->pipes = malloc(sizeof(int *) * mr->data_sets);
	int i;
	for(i = 0; i < mr->data_sets; i++) {
		mr->pipes[i] = malloc(sizeof(int) * 2);
		pipe(mr->pipes[i]);
	}

	// Create epoll events
	mr->epoll_fd = epoll_create(mr->data_sets);
	struct epoll_event event[mr->data_sets];
	memset(event, 0, sizeof(struct epoll_event) * mr->data_sets);	

	// Create child processes to call map function
	for(i = 0; i < mr->data_sets; i++) {
		int read_fd = mr->pipes[i][0];
		int write_fd = mr->pipes[i][1];
		if(fork() == 0) {
			// We are in child process
			close(read_fd);
			mr->map(write_fd, values[i]);
			exit(0);
		}
		else {
			// We are in parent process
			close(write_fd);
		}
		
		// Setup epoll event for process
		event[i].events = EPOLLIN;
		event[i].data.fd = read_fd;
		epoll_ctl(mr->epoll_fd, EPOLL_CTL_ADD, read_fd, &event[i]);
	}

	// Create worker thread to process reduce
	pthread_create(&(mr->worker), NULL, thread_reduce, (void *)mr);

	// Return before child processes and worker thread finish
	return;
}


/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	pthread_join(mr->worker, NULL);
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	return datastore_get(mr->dictionary, result_key, NULL);
}


/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr)
{
	int i;
	for(i = 0; i < mr->data_sets; i++) {
		free(mr->buffer[i]);
		free(mr->pipes[i]);
	}
	free(mr->buffer);
	free(mr->pipes);
	datastore_destroy(mr->dictionary);
	free(mr->dictionary);	
}
