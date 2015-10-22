/** @file parmake.c */
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <semaphore.h>

#include "queue.h"
#include "parser.h"

/*
 * Entry point to parmake.
 */

// Global Variables
queue_t *queue;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int rules_left;

// Structure for rule
typedef struct rule_t {
	char *target;
	queue_t *deps;
	queue_t *commands;
	int ran;	// 0 if untouched, 1 if in use, 2 if finished
} rule_t;

/* Helping functions for parser*/

// Function for finding target to append cmds and deps
rule_t *find_target(char *target) {
        rule_t *rule = NULL;
        int i = 0;
        while(i < queue_size(queue)) {
                rule_t *temp = queue_at(queue, i);
                if(strcmp(target, temp->target) == 0) {
                        rule = temp;
                        break;
                }
                i++;
        }
        if(rule == NULL)
                exit(EXIT_FAILURE);
        return rule;
}

// Callback function for parser, makes new rule and adds rule to queue
void parsed_new_target(char *target) {
	rule_t *rule = malloc(sizeof(rule_t));
	rule->deps = malloc(sizeof(queue_t));
	queue_init(rule->deps);
	rule->commands = malloc(sizeof(queue_t));
	queue_init(rule->commands);
	rule->ran = 0;
	rule->target = malloc(sizeof(char) * (strlen(target) + 1));
	strcpy(rule->target, target);
	queue_enqueue(queue, rule);
}

// Callback function for parser, adds dependency to rule
void parsed_new_dependency(char *target, char *dependency) {
	rule_t *rule = find_target(target);
	char *dep = malloc(sizeof(char) * (strlen(dependency) + 1));
	strcpy(dep, dependency);
	queue_enqueue(rule->deps, dep);
}

// Callback function for parser, adds command to rule
void parsed_new_command(char *target, char *command) {
        rule_t *rule = find_target(target);
	char *cmd = malloc(sizeof(char) * (strlen(command) + 1));
	strcpy(cmd, command);
	queue_enqueue(rule->commands, cmd);
}

/* Helper functions for threading & running */

// Function to compare modification times of two files
double mod_time_compare(char *file, char *target) {
	struct stat stat_1, stat_2;
	if(stat(file, &stat_1) == -1)
		exit(EXIT_FAILURE);
	if(stat(target, &stat_2) == -1)
		exit(EXIT_FAILURE);
	return difftime(stat_1.st_mtime, stat_2.st_mtime);
}

// Function to run commands of rule
void run_command(rule_t *rule) {
	int i;
	for(i = 0; i < queue_size(rule->commands); i++) {
		if(system(queue_at(rule->commands, i)) != 0)
			exit(EXIT_FAILURE);
	}
}

// Function to check if dependencies have been met, return 1 if dependencies are met, 0 otherwise
int dependencies_met(rule_t *rule) {

	// If there are no dependencies rule is ready
	if(queue_size(rule->deps) == 0)
		return 1;

	// Check if all dependencies are files, if so, check modification time
	int all_deps_files = 1;
	int i;
	for(i = 0; i < queue_size(rule->deps); i++) {
		if(access(queue_at(rule->deps, i), F_OK) != 0) {
			all_deps_files = 0;
			break;
		}
	}
	
	// Special instructions if all dependencies are files
	if(all_deps_files) {

		// If rule isn't a file, run the rule
		if(access(rule->target, F_OK) != 0)
			return 1;

		// If the mod time of any dependency file is newer than the rule file, run the rule
		int j;
		for(j = 0; j < queue_size(rule->deps); j++) {
			if(mod_time_compare(queue_at(rule->deps, j), rule->target) >= 0)
				return 1;				
		}

		// This means that the rule is newer than all files, so don't run the rule but mark as finished
		rule->ran = 2;
		rules_left--;
		pthread_cond_broadcast(&cond);
		return 0;
	}

	// Check that dependencies are either files or rules that have been run
	for(i = 0; i < queue_size(rule->deps); i++) {

		// If the dependency is not a file and is not a finished rule, don't run the rule
		if(access(queue_at(rule->deps, i), F_OK) != 0 && find_target(queue_at(rule->deps, i))->ran != 2)
			return 0;
	}

	// Otherwise the rule is ready to be run
	return 1;
}

// Function to grab rule to be run
rule_t *grab_rule() {

	// Picks a rule with met dependencies that has not been ran
	rule_t *rule = NULL;
	int i;
	for(i = 0; i < queue_size(queue); i++) {
		rule_t *temp = queue_at(queue, i);

		// Check that rule is not finished or taken, then check if dependencies are met
		if(temp->ran == 0 && dependencies_met(temp) == 1) {
			rule = temp;
			break;
		}
	}
	return rule;
}

// Function to run rules
void *run_rule(void *args) {

	// Fetch rules while there are any left
	rule_t *rule;
	pthread_mutex_lock(&lock);
	while(rules_left) {

		// While there are rules left and none are available, wait for something to change
		while(rules_left && (rule = grab_rule()) == NULL && rules_left) { // Rules left is evaluated twice because function can change value
				pthread_cond_wait(&cond, &lock);
		}

		// If what changed was that we ran out of rules, end thread 
		if(rules_left == 0) {
			break;
		}

		// Otherwise mark the rule as taken
		rules_left--;
		rule->ran = 1;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&lock);

		// Run the command
		run_command(rule);
		
		// Add rule to the ran queue and broadcast that a rule is finished
		pthread_mutex_lock(&lock);
		rule->ran = 2;
		pthread_cond_broadcast(&cond);
	}

	// Signal other threads to exit as well
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&lock);
	return NULL;
}

/* Print Queue */

// Function to print queue (debugging)
void print_queue() {
	int k;
        for(k = 0; k < queue_size(queue); k++) {
                rule_t *rule = queue_at(queue, k);
                printf("Target : %s\n", rule->target);
                int j;
                printf("Dependencies:\n");
                for(j = 0; j < queue_size(rule->deps); j++) {
                        char *dep = queue_at(rule->deps, j);
                        printf("%s\n", dep);
                }
                printf("Commands: ");
                for(j = 0; j < queue_size(rule->commands); j++) {
                        char *cmd = queue_at(rule->commands, j);
                        printf("%s\n", cmd);
                }
        }
}

/* Main Function */

// Parses command line into rules and parallel processes the rules
int main(int argc, char **argv)
{
	/* Parse command line */

	// Define makefile and thread count through options
	char *makefile = NULL;
	int thread_num = 1;
	int opt;

	while((opt = getopt(argc, argv, "f:j:")) != -1) {
		switch(opt) {
			case 'f':
				makefile = optarg;
				break;
			case 'j':
				thread_num = atoi(optarg);
				break;
			default:
				fprintf(stderr, "Usage: %s [ -f makefile ] [ -j threads ] [ targets ]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// Define makefile if not already defined
	if(makefile == NULL) {
		if(access("makefile", F_OK) == 0)
			makefile = "makefile";
		else if(access("Makefile", F_OK) == 0)
			makefile = "Makefile";
		else exit(EXIT_FAILURE);
	}

	// Create NULL terminated array of targets
	char **targets = malloc(sizeof(char *) * (argc - optind + 1));
	int target_num = 0;
	while(1) {
		if(optind == argc) {
			targets[target_num] = NULL;
			break;
		}
		targets[target_num] = argv[optind];
		optind++;
		target_num++;
	}

	/* Parse Makefile */

	// Initialize rule queue and pass makefile and queueing functions to parsing function
	queue = malloc(sizeof(queue_t));
	queue_init(queue);
	parser_parse_makefile(makefile, targets, &parsed_new_target, &parsed_new_dependency, &parsed_new_command);
	free(targets);
	
	/* Multithread run the makefile */
	
	// Create threads
	rules_left = queue_size(queue);	
	pthread_t threads[thread_num];

	int i;
	for(i = 0; i < thread_num; i++) {
		pthread_create(&threads[i], NULL, &run_rule, NULL);
	}

	// Join threads
	for(i = 0; i < thread_num; i++) {
		pthread_join(threads[i], NULL);
	}

	/* Free Memory */

	// Traverse queue and delete all associated memory
	int f;
	for(f = 0; f < queue_size(queue); f++) {
		rule_t *delete = queue_at(queue, f);
		free(delete->target);
		int d;
		for(d = 0; d < queue_size(delete->deps); d++) {
			char *dep = queue_at(delete->deps, d);
			free(dep);
		}
		for(d = 0; d < queue_size(delete->commands); d++) {
			char *cmd = queue_at(delete->commands, d);
			free(cmd);
		}
		queue_destroy(delete->deps);
		free(delete->deps);
		queue_destroy(delete->commands);
		free(delete->commands);
		free(delete);
	}
	queue_destroy(queue);
	free(queue);	
	
	// Return
	return 0;
}

