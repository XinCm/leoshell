#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "log.h"
#include "cmd.h"
#include "readline.h"
#include "history.h"
#define DELIM " \t"
#define DEBUG 0


const char* builtin_cmd[]={".","..","alias","bg","bind","break","for","while","cd","source","export","history"};
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

bool string_exists(const char* list[], size_t length, const char* target) {
    for (size_t i = 0; i < length; i++) {
        if (strcmp(list[i], target) == 0) {
            return true;
        }
    }
    return false;
}

int cmd_handle(char** _argv, int argc){

    if(string_exists(builtin_cmd,sizeof(builtin_cmd) / sizeof(builtin_cmd[0]), _argv[0])){
        cmd_builtin(_argv,argc);  //内置cmd,不fork新进程
    } else {
        cmd_common(_argv,argc);  //常规cmd,fork新进程
    }
    return 0;
}

int main(void){

    char* _argv[256];
    char *command;
    path_init();
    while(1){
        printf_host_name();
        fflush(stdout);   //强制刷新缓冲区
        // fgets(command,sizeof(command),stdin);
        command = readline(">> ");
        if(!command) break;
        add_history(command);
        int argc = split(command, _argv); //以空格切割输入字符
        if(!argc)
            continue;
        cmd_handle(_argv,argc);
        free(command);
    }
}