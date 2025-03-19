#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include "log.h"
#include "cmd.h"

char* static_commands[]={"alias","bg","bind","break","for","while","cd","source","export","history",NULL};
// 动态命令存储结构
typedef struct {
    char **commands;
    int count;
    int capacity;
} CommandList;

CommandList dynamic_commands = {0};

/* 补全生成器 */
char *command_generator(const char *text, int state) {
    static int list_index, len;
    static int current_list = 0; // 0=静态, 1=动态
    char *name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
        current_list = 0;
    }

    // 遍历静态命令
    while (current_list == 0) {
        name = static_commands[list_index++];
        if (!name) {
            current_list = 1;
            list_index = 0;
            break;
        }
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    // 遍历动态命令
    while (current_list == 1 && list_index < dynamic_commands.count) {
        name = dynamic_commands.commands[list_index++];
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL;
}

static int is_path_completion(const char *text) {
    // 包含路径分隔符或以特殊字符开头
    return strchr(text, '/') ||
          (text[0] == '.' && text[1] == '/') ||  // ./开头
          (text[0] == '.' && text[1] == '.' && text[2] == '/') || // ../开头
          text[0] == '~';                        // 家目录
}

char **custom_completion(const char *text, int start, int end) {

    if (start > 0 || is_path_completion(text)) {
        // 参数位置或路径特征时使用文件补全
        rl_attempted_completion_over = 0;
        return NULL;
    }
    // 命令位置使用自定义补全
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
    }

/* 初始化动态命令列表 */
void init_command_list() {
    dynamic_commands.capacity = 16;
    dynamic_commands.commands = malloc(dynamic_commands.capacity * sizeof(char *));
    dynamic_commands.count = 0;
}

/* 添加命令到动态列表（自动去重） */
void add_command(const char *name) {
    // 去重检查
    for (int i = 0; i < dynamic_commands.count; i++) {
        if (strcmp(dynamic_commands.commands[i], name) == 0) {
            return;
        }
    }

    // 扩容检查
    if (dynamic_commands.count >= dynamic_commands.capacity) {
        dynamic_commands.capacity *= 2;
        dynamic_commands.commands = realloc(dynamic_commands.commands,
                                          dynamic_commands.capacity * sizeof(char *));
    }

    dynamic_commands.commands[dynamic_commands.count++] = strdup(name);
}

/* 从单个目录加载可执行文件 */
void process_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        // 构建完整路径
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // 检查文件属性
        struct stat st;
        if (stat(full_path, &st) == 0 &&
            S_ISREG(st.st_mode) &&
            access(full_path, X_OK) == 0) {
            add_command(entry->d_name);
        }
    }
    closedir(dir);
}

/* 处理PATH环境变量 */
void load_commands_from_path(const char *env_var) {
    const char *path = getenv(env_var);
    if (!path) {
        fprintf(stderr, "环境变量 %s 未设置\n", env_var);
        return;
    }

    // 复制字符串用于分割
    char *path_copy = strdup(path);
    char *dir = strtok(path_copy, ":");

    while (dir) {
        process_directory(dir);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
}

void complete_cmd(){
    init_command_list();
    load_commands_from_path("PATH");
    rl_attempted_completion_function = custom_completion;
    rl_completer_word_break_characters = " \t\n\"\\'`@$><=;|&{(";
    rl_completion_append_character = '\0';
}