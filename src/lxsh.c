#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include "log.h"
#include "cmd.h"

#define MAX_HISTORY 500 
#define DEBUG 0
void flush_input_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH); // 清空标准输入缓冲区
}
// 信号处理函数
void handle_sigint(int sig) {
    (void)sig;

    // 强制换行并重置状态
    write(STDOUT_FILENO, "\n", 1);  // 异步安全换行
    rl_reset_line_state();          // 重置 Readline 行状态
    rl_replace_line("", 0);         // 清空当前输入内容
    printf_host_name();
    rl_redisplay();                 // 刷新显示提示符
    flush_input_buffer();           // 清空输入缓冲区（解决首字符丢失）
}

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
    char *command;
    char* _argv[256];
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    signal(SIGINT, handle_sigint);

    misc_init();
    complete_cmd();
    while(1){
        printf_host_name();
        // fflush(stdout);   //强制刷新缓冲区
        command = readline(">> ");

        if(!command)    break;
        add_history(command);
        write_history(HISTORY_FILE);

        int argc = split(command, _argv);
        if(!argc)
            continue;
        cmd_handle(_argv,argc);
        free(command);
    }
}