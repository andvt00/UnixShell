#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_LENGTH	80			// The maximum length command
#define DEF_ARGS MAX_LENGTH/2	// The default number of arguments
#define SINGLE_COMMAND 0
#define INPUT_REDIRECTION 1
#define OUTPUT_REDIRECTION 2

int cd(char* path); // cd command
// Command's type checking
bool hasAmpersandAtLast(char** args); // check if command has &
int check_input_type(char* input_line, char** cmd1, char** cmd2); // check if command is single or i/o redirection
int isPipeCommand(char** args); // check if command is a pipe
// Parsing
char** parse_arguments(char* command); // split command string to arguments
char** parse_redirection(char* input_line); // split i/o redirection to command and file name
// Command executing
void exec_command(char* command);  // execute command
void input_redirection(char* token1, char* token2); // input redirection handling
void output_redirection(char* token1, char* token2); // output redirection handling
bool execPipeCommand(char** cmd1, char** cmd2, bool hasAmpersand); // execute pipe command

int main() {
	char input_line[MAX_LENGTH];
	int should_run = 1; /* flag to determine when to exit program */
	char shell_name[] = "osh> ";
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
		if (strcmp("exit", args[0]) == 0) {
			free(args);
			free(temp);
			should_run = 0;
			break;
		}
		if (strcmp("!!", args[0]) != 0) {
			if (history) 
				free(history);
			history = (char*)malloc((strlen(input_line) + 1) * sizeof(char));
			strcpy(history, input_line); // copy command to history
			strcpy(temp, input_line);
		}
		if (strcmp("!!", args[0]) == 0) { // call command from history buffer
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
			fflush(stdout);
		}
		free(temp);
	}
	return 0;
} 

int cd(char* path) {
	return chdir(path);
}

bool hasAmpersandAtLast(char** args) {
	if (!args[0])
		return false;
	int size = 1;
	while (args[size])
		size++;
	if (strcmp("&", args[size-1]) == 0) {
		args[size - 1] = NULL; // delete the last "&" argument
		return true;
	}
	return false;
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

int isPipeCommand(char** args) {
	// Check if the command only has one pipe character and no other redirection operators
	int i = 0;
	int nPipeChar = 0;
	int pos = -1;
	while(args[i] != NULL)
	{
		if (strcmp("<", args[i]) == 0 || strcmp(">", args[i]) == 0 ) {
			return false;
		}
		else if (strcmp("|", args[i]) == 0) {
			nPipeChar++;
			pos = i;
		}
		i++;
	}
	if (nPipeChar==1)
	{
		// Asign the pipe argument as NULL to divide between 2 commands
		args[pos] = NULL;
		return pos;
	}
	return -1;
}

char** parse_arguments(char* command) {
	char delim[] = " \n\t\r\a";
	unsigned int args_size = DEF_ARGS;
	char** args = (char**)malloc(args_size * sizeof(char*));
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

void exec_command(char* command) {
	if (strcmp(command, "") == 0) {
		return;
	}
	char** args = parse_arguments(command);
	// Check if the last argurment is "&" or not (by using the "hasAmpersand" variable)
	// If the last argurment is "&" then the parent and child processes will run concurrently
	if (strcmp(args[0], "cd") == 0) {
		if (cd(args[1]) < 0) {
			perror(args[0]);
		}
		free(args); // free up memory for arguments
		return;
	}
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
		// Fork a child process 
		pid_t child_pid = fork();
		if (child_pid < 0) {
			// Parent process, but child couldn't be created
			perror("Fork failed");
			free(args); // free up memory for arguments
			exit(EXIT_FAILURE);
		}

		if (child_pid == 0) { 
			// Child process
			if (execvp(args[0], args) < 0) { // catch error
				perror(args[0]);
				free(args); // free up memory for arguments
				exit(EXIT_FAILURE);
			}
		} 
		else if (child_pid > 0) {
			// Parent process
			int status = 0;
			pid_t wpid;
			// Process wait if command have &
			if (!hasAmpersand) {
				while((wpid = wait(&status)) > 0);
			}
		}
	}
	free(args);
}

void input_redirection(char* token1, char* token2) {
	char** commands = parse_arguments(token1);
	char** filenames = parse_arguments(token2);
	bool hasAmpersand = hasAmpersandAtLast(filenames);
	pid_t child_pid = fork();
	if (child_pid < 0) {
		// Parent process, but child couldn't be created
		perror("Fork failed");
		free(commands); // free up memory for arguments
		free(filenames);
		exit(EXIT_FAILURE);
	}

	if (child_pid == 0) { 
		// Child process
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
		// Parent process
		int status = 0;
		pid_t wpid;
		// Process wait if command have &
		if (!hasAmpersand) {
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
		// Parent process, but child couldn't be created
		perror("Fork failed");
		free(commands); // free up memory for arguments
		free(filenames);
		exit(EXIT_FAILURE);
	}

	if (child_pid == 0) { 
		// Child process
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
		// Parent process
		int status = 0;
		pid_t wpid;
		// Process wait if command have &
		if (!hasAmpersand) {
			while((wpid = wait(&status)) > 0);
		}
	}
	free(commands);
	free(filenames);
}

bool execPipeCommand(char** cmd1, char** cmd2, bool hasAmpersand)
{
	int pd[2]; // 0 end is reading, 1 end is writing
	if (pipe(pd) < 0)
	{
		printf("Cannot create pipe!\n");
		return 0;
	}
	pid_t pid2, pid1; 
	pid1 = fork();
	if (pid1 < 0)
	{
		printf("Cannot fork first command\n");
		return 0;
	}
	if (pid1 == 0)
	{
		// child 1 process
		// we only need to write here so we close the reading end
		close(pd[0]);
		// copying the file descriptor of STD_OUT to pd[1]
		dup2(pd[1], STDOUT_FILENO);
		// descriptor alreadly duplicated, the old pd[1] is no longer needed
		close(pd[1]);

		if (execvp(cmd1[0], cmd1) < 0)
		{
			perror(cmd1[0]);
			return 0;
		}
	}
	else
	{
		// parent process
		pid2 = fork();
		if (pid2 < 0)
		{
			printf("Cannot fork second command\n");
			return 0;
		}

		if (pid2 == 0)
		{
			// child 2 process
			// we only need to read here so we close the writing end
			close(pd[1]);
			// copying the file descriptor of STD_IN to pd[0]
			dup2(pd[0], STDIN_FILENO);
			close(pd[0]);
			if (execvp(cmd2[0], cmd2) < 0)
			{
				perror(cmd2[0]);
				return 0;
			}
		}
		else
		{
			// Process wait if command have &
			if (!hasAmpersand)
			{
				waitpid(pid1, NULL, 0); 
				waitpid(pid1, NULL, 0); 
			}
			// Since file descriptors are inherited(copied) when calling fork(), they still exits in the parent process
			// Therefore, we have to close the pipe
			close(pd[0]);
			close(pd[1]);
		}

	}
	// Since file descriptors are inherited (copied) when calling fork()
	// We have to close the pipe
	close(pd[0]);
	close(pd[1]);
	return true;

}