#include "pipe.h"


bool execPipeCommand(char **cmd1, char **cmd2, bool hasAmpersand)
{
	int pd[2]; //0 end is reading, 1 end is writing
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
		//child 1 process
		//we only need to write here so we close the reading end
		close(pd[0]);
		//copying the file descriptor of STD_OUT to pd[1]
		dup2(pd[1], STDOUT_FILENO);
		//descriptor alreadly duplicated, the old pd[1] is no longer needed
		close(pd[1]);

		if (execvp(cmd1[0], cmd1) < 0)
		{
			perror(cmd1[0]);
			return 0;
		}
	}
	else
	{
		//parent process
		pid2 = fork();
		if (pid2 < 0)
		{
			printf("Cannot fork second command\n");
			return 0;
		}

		if (pid2 == 0)
		{
			//child 2 process
			//we only need to read here so we close the writing end
			close(pd[1]);
			//copying the file descriptor of STD_IN to pd[0]
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
			//Process wait if command have &
			if (!hasAmpersand)
			{
				waitpid(pid1, NULL, 0); 
            	waitpid(pid1, NULL, 0); 

			}
			//Since file descriptors are inherited(copied) when calling fork(), they still exits in the parent process
			//Therefore, we have to close the pipe
			close(pd[0]);
            close(pd[1]);
		}

	}
	//Since file descriptors are inherited(copied) when calling fork()
	//We have to close the pipe
	close(pd[0]);
    close(pd[1]);
	return true;

}