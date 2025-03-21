#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "log.h"
#include "cmd.h"
#define MAX_CMD_LEN 1024
#define DELIM " \t"
char home_path[128];
char lash_path[2][128];

int flag_out = 0;
int flag_in = 0 ;
int misc_init(){
    memset(lash_path,0,sizeof(lash_path));

    int ret = snprintf(home_path, sizeof(home_path), "/home/%s", getenv("USER"));
    if (ret < 0 || (size_t)ret >= sizeof(home_path)) {
        lprintf(RED, "路径过长\n");
        return -1;
    }


    if (read_history(HISTORY_FILE) != 0) {
        chdir(home_path);
        FILE *fp = fopen(HISTORY_FILE, "a");
        if (fp) fclose(fp);
    }
    stifle_history(1000);

    return 0;
}

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
    lprintf(PINK,"%s ",full_name);
    return 0;
}


static void call_cd( char **argv, int argc){
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

static void call_export(char** _argv, int argc){
    for(int i = 1; i < argc; i++){
        if (strchr(_argv[i], '=') == NULL) {
            fprintf(stderr, "Invalid export format: %s (must be VAR=value)\n", _argv[i]);
            continue;
        }
        char *env_entry = strdup(_argv[i]);
        if (!env_entry) {
            perror("strdup failed");
            continue;
        }
        if (putenv(env_entry) != 0) {
            perror("putenv failed");
            free(env_entry);
        }
    }
}

static void call_source(char** _argv, int argc){
    FILE *fp = fopen(_argv[1], "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char *cmd[128];
    char line[65535];

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || strlen(line) < 2) continue; //skip # space
        split(line,cmd);

        if(!cmd_builtin(cmd,argc)){
            cmd_common(cmd,argc);
        }
    }
    fclose(fp);
    return;
}

static void call_history(void){
    read_history(HISTORY_FILE);
    HIST_ENTRY **hist = history_list();
    if (hist) {
        for (int i = 0; hist[i]; i++) {
            printf("%d: %s\n", i+1, hist[i]->line);
        }
    }
}

struct cmd_info{
    char *argv[16];
    char in[128];
    char out[128];
}cmd[16];

static void do_pipe(char** _argv){
    int pipe_num = 0;
    int fd[16][2];  //max 16 pip
    char **current_arg = _argv; // 指向当前参数

    while (*current_arg) {
        if (strcmp(*current_arg, "|") == 0) pipe_num++;
        current_arg++;
    }
    current_arg = _argv; // 重置指针

    if (pipe_num >= 16) {
        fprintf(stderr, "Error: Too many pipes (max 16)\n");
        return;
    }

    // Step 2: 创建管道
    for (int i = 0; i < pipe_num; i++) {
        if (pipe(fd[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
    // Step 3: 创建子进程
    for (int cnt = 0; cnt <= pipe_num; cnt++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) { // 子进程
            // 重定向输入输出
            if (cnt > 0) dup2(fd[cnt-1][0], STDIN_FILENO);
            if (cnt < pipe_num) dup2(fd[cnt][1], STDOUT_FILENO);

            // 关闭所有管道端
            for (int j = 0; j < pipe_num; j++) {
                close(fd[j][0]);
                close(fd[j][1]);
            }

            // 收集当前命令参数
            char *cmd_args[16];
            int arg_count = 0;
            while (*current_arg && strcmp(*current_arg, "|") != 0) {
                cmd_args[arg_count++] = *current_arg;
                current_arg++;
            }
            cmd_args[arg_count] = NULL;

            // 执行命令
            execvp(cmd_args[0], cmd_args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { // 父进程
            // 移动指针到下一个命令
            while (*current_arg && strcmp(*current_arg, "|") != 0) current_arg++;
            if (*current_arg) current_arg++;
        }
    }

    for (int j = 0; j < pipe_num; j++) {
        close(fd[j][0]);
        close(fd[j][1]);
    }
    for (int cnt = 0; cnt <= pipe_num; cnt++) {
        wait(NULL);
    }
}

static void do_outre(char** _argv){

}

int cmd_builtin(char** _argv, int argc){

    for(int i = 0; i < argc; i++){
        if(strcmp(_argv[i],"|") == 0){
            do_pipe(_argv);
            return 0;
        }
    }
    for(int i = 0; i < argc; i++){
        if(strcmp(_argv[i],">") == 0){
            do_outre(_argv);
            return 0;
        }
    }

    if(strcmp(_argv[0],"cd") == 0){
        call_cd(_argv,argc);
        return 0;
    } else if(strcmp(_argv[0],"export") == 0){
        call_export(_argv,argc);
        return 0;
    } else if(strcmp(_argv[0],"source") == 0){
        call_source(_argv,argc);
        return 0;
    } else if(strcmp(_argv[0],"history") == 0){
        call_history();
        return 0;
    } 
    return -1;
}

int cmd_common(char** _argv, int argc){

    if(strcmp(_argv[0],"ls") == 0){
        _argv[argc] = "--color";
        _argv[argc+1] = NULL;
    }

    pid_t id = fork();
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
