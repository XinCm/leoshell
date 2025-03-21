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
#include <errno.h>
#include "log.h"
#include "cmd.h"

#define MAX_HISTORY 500 
#define DEBUG 0

char *full_name = NULL;
char host_name[256];
static const char *color_start = "\001\033[38;2;255;192;203m\002";
static const char *color_end = "\001\033[0m\002";

static const char *cached_user = NULL;
static char cached_host[256] = {0};

void init_cache() {
    cached_user = getenv("USER");
    if (!cached_user) cached_user = "unknown";

    if (gethostname(cached_host, sizeof(cached_host)) != 0) {
        strcpy(cached_host, "unknown");
    }
}

char* get_cur_path() {
    static char *cur_path = NULL;
    static size_t buf_size = 0;

    if (!cur_path) {
        buf_size = 1024;
        cur_path = malloc(buf_size);
    }

    while (getcwd(cur_path, buf_size) == NULL) {
        if (errno != ERANGE) {
            strcpy(cur_path, "?");
            break;
        }
        buf_size *= 2;
        cur_path = realloc(cur_path, buf_size);
    }

    // 替换主目录为 ~
    const char *home = getenv("HOME");
    if (home && strncmp(cur_path, home, strlen(home)) == 0) {
        static char short_path[1024];
        snprintf(short_path, sizeof(short_path), "~%s", cur_path + strlen(home));
        return short_path;
    }

    return cur_path;
}

void get_prompt() {
    const char *cur_path = get_cur_path();

    // 仅路径变化时重新分配
    static int last_len = 0;
    int needed_len = snprintf(NULL, 0, "%s%s@%s:%s$ %s",
                             color_start, cached_user, cached_host, cur_path, color_end);

    if (!full_name || needed_len > last_len) {
        free(full_name);
        full_name = malloc(needed_len + 1);
        last_len = needed_len;
    }

    snprintf(full_name, needed_len + 1, "%s%s@%s:%s$ %s",
            color_start, cached_user, cached_host, cur_path, color_end);
}

void flush_input_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH); // 清空标准输入缓冲区
}
// 信号处理函数
void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);  // 异步安全换行
    rl_reset_line_state();          // 重置 Readline 行状态
    rl_replace_line("", 0);         // 清空当前输入内容
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

    init_cache();
    misc_init();
    complete_cmd();

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    signal(SIGINT, handle_sigint);

    while(1){
        get_prompt();
        command = readline(full_name);

        if(!command)    break;
        add_history(command);
        write_history(HISTORY_FILE);

        int argc = prase(command, _argv);
        if(!argc)
            continue;
        cmd_handle(_argv,argc);
        free(command);
    }
    free(full_name);
    return 0;
}