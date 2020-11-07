#ifndef HEADER_MAIN
#define HEADER_MAIN

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h> //type bool
#include "pipe.h"

#endif

#define MAX_LENGTH	80 // The maximum length command
#define DEF_ARGS	MAX_LENGTH/2 //The maximum number of arguments
#define SINGLE_COMMAND 0
#define INPUT_REDIRECTION 1
#define OUTPUT_REDIRECTION 2
// char temp[MAX_LENGTH];

int isPipeCommand(char **args){
	//Check if the command only has one pipe character and no other redirection operators
	int i = 0;
	int nPipeChar = 0;
	int pos = -1;
	while(args[i] != NULL)
	{
		if (strcmp("<", args[i])==0 || strcmp(">", args[i])==0 ){
			return false;
		}
		else if (strcmp("|", args[i])==0){
			nPipeChar++;
			pos = i;
		}
		i++;
	}
	if (nPipeChar==1)
	{
		//asign the pipe argument as NULL to divide between 2 commands
		args[pos] = NULL;
		return pos;
	}
	return -1;
}


//split string command to arguments
char** parse_arguments(char *command) {
	char delim[] = " \n\t\r\a";
	unsigned int args_size = DEF_ARGS;
	char** args = (char**)malloc(args_size * sizeof(char*));
	// strcpy(temp, command);
    char* param;
    param = strtok(command, delim);
    int size = 0;
    while (param != NULL) {
       	args[size] = param;
		if (++size >= args_size) {
			args_size += DEF_ARGS;
			args = (char**)realloc(args, args_size * sizeof(char*));
			if (!args) {
				perror("Allocaion error");
				exit(EXIT_FAILURE);
			}
		}
        param = strtok(NULL, delim);
    }
    args[size] = NULL;
    return args;
}

bool hasAmpersandAtLast(char** args){
	if (!args[0])
		return false;
	int size = 1;
	while (args[size])
		size++;
	if (strcmp("&", args[size-1])==0){
    	args[size-1] = NULL; //delete the last "&" argument
    	return true;
    }
    return false;
}

int cd(char *path) {
    return chdir(path);
}


void exec_command(char* command) {
	if (strcmp(command, "") == 0) {
		return;
	}
	char** args = parse_arguments(command);
	//check if the last argurment is "&" or not (by using the "hasAmpersand" variable)
	//if the last argurment is "&" then the parent and child processes will run concurrently
	bool hasAmpersand = hasAmpersandAtLast(args);

	int pipePos = isPipeCommand(args);
	if (pipePos > 0)
	{
		if (!execPipeCommand(args, args + pipePos + 1, hasAmpersand))
		{
			free(args); // free up memory for arguments
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		//Fork a child process 
		pid_t child_pid = fork();
		if (child_pid < 0) {
			//Parent process, but child couldn't be created
			perror("Fork failed");
			free(args); // free up memory for arguments
			exit(EXIT_FAILURE);
		}

		if (child_pid == 0) { 
			//Child process
			if (execvp(args[0], args) < 0) { // catch error
				perror(args[0]);
				free(args); // free up memory for arguments
				exit(EXIT_FAILURE);
			}
		} 
		else if (child_pid > 0) {
			//Parent process
			int status = 0;
			pid_t wpid;
			//Process wait if command have &
			if (!hasAmpersand){
				while((wpid = wait(&status)) > 0);
			}
		}
	}
	free(args);
}

char** parse_redirection(char* input_line) {
	char delim[] = "<>";
	char** cmd_list = (char**)malloc(3*sizeof(char*));
	char* token = strtok(input_line, delim);
	cmd_list[0] = token;
	token = strtok(NULL, "\n");
	cmd_list[1] = token;
	cmd_list[2] = NULL;
	return cmd_list;
}

int check_input_type(char* input_line, char** cmd1, char** cmd2) {
	int type = SINGLE_COMMAND;
	char** cmd_list = NULL;
	if (strchr(input_line, '<') && strchr(input_line, '>')) {
		*cmd1 = NULL;
		*cmd2 = NULL;
		return -1;
	}
	if (strchr(input_line, '<')) 
		type = INPUT_REDIRECTION;
	else if (strchr(input_line, '>'))
		type = OUTPUT_REDIRECTION;
	if (type != SINGLE_COMMAND) {
		cmd_list = parse_redirection(input_line);
		*cmd1 = cmd_list[0];
		*cmd2 = cmd_list[1];
		free(cmd_list);
	}
	else {
		*cmd1 = input_line;
		*cmd2 = NULL;
	}
	return type;
}

void input_redirection(char* token1, char* token2) {
	char** commands = parse_arguments(token1);
	char** filenames = parse_arguments(token2);
	bool hasAmpersand = hasAmpersandAtLast(filenames);
	pid_t child_pid = fork();
	if (child_pid < 0) {
		//Parent process, but child couldn't be created
		perror("Fork failed");
		free(commands); // free up memory for arguments
		free(filenames);
		exit(EXIT_FAILURE);
	}

	if (child_pid == 0) { 
		//Child process
		int fd = open(filenames[0], O_RDONLY);
		if (fd < 0) {
			perror("Open failed");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
			perror("dup2 failed");
			exit(EXIT_FAILURE);
		}
		close(fd);
		if (execvp(commands[0], commands) < 0) { // catch error
			perror(commands[0]);
			free(commands); // free up memory for arguments
			free(filenames);
			exit(EXIT_FAILURE);
		}
	} 
	else if (child_pid > 0) {
		//Parent process
		int status = 0;
		pid_t wpid;
		//Process wait if command have &
		if (!hasAmpersand){
			while((wpid = wait(&status)) > 0);
		}
	}
	free(commands);
	free(filenames);
}

void output_redirection(char* token1, char* token2) {
	char** commands = parse_arguments(token1);
	char** filenames = parse_arguments(token2);
	bool hasAmpersand = hasAmpersandAtLast(filenames);
	pid_t child_pid = fork();
	if (child_pid < 0) {
		//Parent process, but child couldn't be created
		perror("Fork failed");
		free(commands); // free up memory for arguments
		free(filenames);
		exit(EXIT_FAILURE);
	}

	if (child_pid == 0) { 
		//Child process
		int fd = creat(filenames[0], S_IRWXU);
		if (fd < 0) {
			perror("Open failed");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDOUT_FILENO) < 0) {
			perror("dup2 failed");
			exit(EXIT_FAILURE);
		}
		close(fd);
		if (execvp(commands[0], commands) < 0) { // catch error
			perror(commands[0]);
			free(commands); // free up memory for arguments
			free(filenames);
			exit(EXIT_FAILURE);
		}
	} 
	else if (child_pid > 0) {
		//Parent process
		int status = 0;
		pid_t wpid;
		//Process wait if command have &
		if (!hasAmpersand){
			while((wpid = wait(&status)) > 0);
		}
	}
	free(commands);
	free(filenames);
}

int main() {
	char input_line[MAX_LENGTH];
	int should_run = 1; /* flag to determine when to exit program */
	char shell_name[] = "ssh> ";
	char empty_history[] = "No commands in history";
	char* history = NULL; // History buffer
	char* temp;
	char* token1 = NULL;
	char* token2 = NULL;
	int type = SINGLE_COMMAND;
	while (should_run) {
		printf("%s", shell_name);
		fflush(stdout);
		fgets(input_line, MAX_LENGTH, stdin);
		temp = (char*)malloc((strlen(input_line) + 1) * sizeof(char));
		strcpy(temp, input_line);
		char** args = parse_arguments(temp);
		if (!args[0]) { // if input is empty
			free(args);
			free(temp);
			continue;
		}
		if (strcmp("exit", args[0]) == 0){
			free(args);
			free(temp);
			should_run = 0;
			break;
		}
		if (strcmp(args[0], "cd") == 0) {
            if (cd(args[1]) < 0) {
                perror(args[0]);
            }
			free(args); // free up memory for arguments
			free(temp);
            continue;
        }
		if (strcmp("!!", args[0]) != 0) {
			if (history) 
				free(history);
			history = (char*)malloc((strlen(input_line) + 1) * sizeof(char));
			strcpy(history, input_line);
			strcpy(temp, input_line);
		}
		if (strcmp("!!", args[0]) == 0) { // Command from history buffer
			if (!history) {
				printf("%s\n", empty_history);
				fflush(stdout);
				free(args);
				free(temp);
				continue;
			}
			else {
				printf("%s%s", shell_name, history);
				fflush(stdout);
				free(temp);
				temp = (char*)malloc((strlen(history) + 1) * sizeof(char));
				strcpy(temp, history);
			}
		}
		free(args);
		type = check_input_type(temp, &token1, &token2);
		switch (type) {
		case SINGLE_COMMAND:
			exec_command(token1);
			break;
		case INPUT_REDIRECTION:
			input_redirection(token1, token2);
			break;
		case OUTPUT_REDIRECTION:
			output_redirection(token1, token2);
			break;
		default:
			token1 = NULL;
			token2 = NULL;
			printf("Invalid/Unsupported command\n");
		}
		free(temp);
	}
	return 0;
} 
