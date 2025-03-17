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

void cmd_ls(char** _argv, int argc){
    pid_t id = fork();
    if(id == 0){
        _argv[argc] = "--color";
        _argv[argc+1] = NULL;
        execvp(_argv[0],_argv);
        exit(0);
    } else if (id > 0) {
        // 父进程等待子进程结束
        waitpid(id, NULL, 0);
    }
}

#endif