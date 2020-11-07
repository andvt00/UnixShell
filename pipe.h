#ifndef PIPE
#define PIPE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h> //type bool


bool execPipeCommand(char **cmd1, char **cmd2, bool hasAmpersand);

#endif