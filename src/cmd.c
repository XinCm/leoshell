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
#define MAX_CMD_LEN 1024
#define DELIM " \t"
char home_path[128];
char lash_path[2][128];
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

int cmd_builtin(char** _argv, int argc){
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
