#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h> //type bool
#define MAX_LENGTH	80 // The maximum length command
#define MAX_ARGS	MAX_LENGTH/2 //The maximum number of arguments
char temp[MAX_LENGTH];

//split string command to arguments
char** parse_arguments(char *command) {
	char **args = (char**)malloc(MAX_ARGS * sizeof(char *));
	strcpy(temp, command);
    char *param;
    param = strtok(temp, " ");
    int size = 0;
    while (param != NULL) {
       	args[size++] = param;
        param = strtok(NULL, " ");
    }
    args[size] = NULL;
    return args;
}

void execCommand(char* command) {
	if (strcmp(command, "") == 0) {
		return;
	}
	char** args = parse_arguments(command);
	//Fork a child process 
	pid_t child_pid = fork();
	bool waitChild = 1;
	if (child_pid < 0) {
		//Parent process, but child couldn't be created
		perror("Fork failed");
		free(args); // free up memory for arguments
		exit(1);
	}

	if (child_pid == 0) { 
		//Child process
		if (execvp(args[0], args) < 0) { // catch error
			perror(args[0]);
			free(args); // free up memory for arguments
			exit(1);
		}
	} 
	else if (child_pid > 0) { 
		//Parent process
		//Process wait if command have &
		if (!waitChild){}
	}
	free(args);
}

int main(void) {
	char command[MAX_LENGTH];
	int should_run = 1; /* flag to determine when to exit program */
	while (should_run) {
		printf("ssh>");
		fflush(stdout);
		fgets(command, MAX_LENGTH, stdin);
		char *pos = strchr(command, '\n');
		if (pos)
			*pos = 0;
		char** args = parse_arguments(command);
		if (strcmp("exit", args[0])==0){
			should_run = 0;
			break;
		}
		free(args);
		execCommand(command);
	}
	return 0;
} 
