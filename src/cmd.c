#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "log.h"
#include "cmd.h"
#define HOME_PATH "/home/%s"  getenv("USER")

#define MAX_CMD_LEN 1024
char home_path[128];
char lash_path[2][128];
int path_init(){
    memset(lash_path,0,sizeof(lash_path));

    int ret = snprintf(home_path, sizeof(home_path), "/home/%s", getenv("USER"));
    if (ret < 0 || (size_t)ret >= sizeof(home_path)) {
        lprintf(RED, "路径过长\n");
        return -1;
    }
    return 0;
}

int printf_host_name(void){
    char host_name[256];
    char full_name[512];
    gethostname(host_name,sizeof(host_name));
    snprintf(full_name,sizeof(full_name),"%s@%s:%s",getenv("USER"),host_name,getcwd(NULL,0));
    lprintf(PINK,"%s ",full_name);
    return 0;
}


void call_cd( char **argv, int argc){
    if(argc == 1){
        chdir(home_path);
    } else if (argc == 2){
        if(strcmp(argv[1],"-") == 0){
            chdir(lash_path[1]);
        }else if(strcmp(argv[1],"~") == 0){
            chdir(home_path);
        }else{
            chdir(argv[1]);
        }
    }else{
        lprintf(RED,"cd: too many arguments\n");
        return;
    }
    memcpy(lash_path[1], lash_path[0], sizeof(lash_path[1]));
    getcwd(lash_path[0],sizeof(lash_path[0]));
}

int cmd_builtin(char** _argv, int argc){
    if(strcmp(_argv[0],"cd") == 0){
        call_cd(_argv,argc);
    } else if(strcmp(_argv[0],"export") == 0){
        for(int i = 1; i < argc; i++){
            putenv(_argv[i]);
        }
    }
    return 0;
}

int cmd_common(char** _argv, int argc){
    pid_t id = fork();
    if(strcmp(_argv[0],"ls") == 0){
        _argv[argc] = "--color";
        _argv[argc+1] = NULL;
    }

    if(id == 0){
        _argv[argc+1] = NULL;
        execvp(_argv[0],_argv);
        exit(0);
    } else if (id > 0) {
        // 父进程等待子进程结束
        waitpid(id, NULL, 0);
    }
    return 0;
}
