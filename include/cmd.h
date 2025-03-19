#ifndef __LEO_CMD__
#define __LEO_CMD__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "log.h"

int path_init();
int cmd_builtin(char** _argv, int argc);
int cmd_common(char** _argv, int argc);
int printf_host_name(void);

#endif