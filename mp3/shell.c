/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "log.h"


log_t Log;
pid_t pid;
char* cwd;
char* line;

void appendToLog(char *line) {
	char* append = strdup(line);
        log_push(&Log, append);
}

void nonBuiltInCommand(char * task) {
	// Append
	appendToLog(task);

	// Parse arguments
	char *argv[16];
	int j;
	for(j = 0; j < 16; j++)
		argv[j] = NULL;
	char *temp = strtok(task, " ");
	int i = 0;
	while(temp != NULL && i < 16) {
		argv[i] = temp;
		temp = strtok(NULL, " ");
		i++;
	}

	// Create new process
	int childStatus;
	pid_t child = fork();
	if(child == 0) {
		printf("Command executed by pid=%d\n", getpid());
		execvp(argv[0], argv);
		
		// Execution failed
		printf("%s: not found\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	else {
		wait(&childStatus);
	}
}

/**
 * Starting point for shell.
 */
int main() {
	// Initialize
	log_init(&Log);
	char buf[256];
	line = (char *)malloc(256);
	size_t lineLength = sizeof(line);

	// Begin taking commands in a while statement that returns if exit is entered
	while(1) {

	// Print working directory	
	printf("(pid=%d)%s$ ", getpid(), getcwd(buf, sizeof(buf)));

	// Get the command and strip newline character
	getline(&line, &lineLength, stdin);
	line[strlen(line) - 1] = 0;
	
	// Check the command against possible inputs
		if(strncmp(line, "exit", 4) == 0) {
			break;
		}
		else if(strncmp(line, "cd ", 3) == 0) {
			// Add to log
			appendToLog(line);

			// Change directory and respond if there is an error
			if(chdir(line + 3) == -1) {
				printf("%s: No such file or directory\n", line + 3);
			}	
			continue;
                }
		else if(strncmp(line, "!#", 2) == 0) {
			// Print the log
			log_print(&Log);
			continue;
                }
		else if(line[0] == '!') {
			// Look up last instance, print and execute
			char *match = log_search(&Log, line + 1);
			if(match != NULL) {
				appendToLog(match);
				printf("%s matches %s\n", line + 1, match);

				// If cd change directory
				if(strncmp(match, "cd ", 3) == 0) {
					if(chdir(match + 3) == -1) {
                                		printf("%s: No such file or directory\n", match + 3);
					}
				}
				// Otherwise run other function
				else nonBuiltInCommand(match);				
			}
			else printf("No Match\n");	
			continue;
		}
		else nonBuiltInCommand(line);
	}

	// Free memory and exit
	free(line);
	log_destroy(&Log);
	return 0;
}
