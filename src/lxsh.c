#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include"../include/log.h"
#define DELIM " \t"
#define DEBUG 0

char command[1024];
int split(char* str,char** _argv){
    int cnt = 0;
    str[strcspn(str, "\n")] = '\0';
    _argv[0] = strtok(str, DELIM);
    if (_argv[0] == NULL) {
        return 0;
    }

    while((_argv[++cnt] = strtok(NULL, DELIM)));

    return cnt;
}
int printf_host_name(void){
    char host_name[256];
    char full_name[512];
    gethostname(host_name,sizeof(host_name));
    snprintf(full_name,sizeof(full_name),"%s@%s:%s",getenv("USER"),host_name,getcwd(NULL,0));
    lprintf(BLUE,"%s ",full_name);
    return 0;
}

int main(void){

    char* _argv[256];

    while(1){
        printf_host_name();
        fflush(stdout);
        fgets(command,sizeof(command),stdin);
        int argc = split(command, _argv);
        if(!argc)
            continue;


        if(strcmp(_argv[0], "ls") == 0) {
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
        if(strcmp(_argv[0], "pwd") == 0) {
            pid_t id = fork();
            if(id == 0){
                execvp(_argv[0],_argv);
                exit(0);
            } else if (id > 0) {
                // 父进程等待子进程结束
                waitpid(id, NULL, 0);
            }
        }

    }
}