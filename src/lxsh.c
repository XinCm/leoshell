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

#define MAX_HISTORY 500 
#define DEBUG 0

bool string_exists(char* list[], size_t length, const char* target) {
    for (size_t i = 0; i < length; i++) {
        if (strcmp(list[i], target) == 0) {
            return true;
        }
    }
    return false;
}

int cmd_handle(char** _argv, int argc){

    if(cmd_builtin(_argv,argc))
        cmd_common(_argv,argc);
    return 0;
}

int main(void){

    char* _argv[256];
    char *command;
    misc_init();
    complete_cmd();
    while(1){
        printf_host_name();
        fflush(stdout);   //强制刷新缓冲区

        command = readline(">> ");
        if(!command) break;
        add_history(command);
        write_history(HISTORY_FILE);

        int argc = split(command, _argv); //以空格切割输入字符
        if(!argc)
            continue;
        cmd_handle(_argv,argc);
        free(command);
    }
}