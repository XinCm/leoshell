#ifndef __LEO_LOG__
#define __LEO_LOG__
#include <stdio.h>
#include <stdarg.h>

typedef struct {
    const char *name;
    const char *code;
} Color;

#define BLUE    "\033[34m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define PINK    "\033[38;2;255;192;203m" //\033[35m
#define BOLD    "\033[1m"
#define RESET   "\033[0m"


#define lprintf(color, fmt, ...) \
    printf("%s" fmt "%s", color, ##__VA_ARGS__, RESET)

#endif