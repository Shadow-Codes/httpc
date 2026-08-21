#ifndef SERVER_H
#define SERVER_H

#include "mime.h"
typedef struct fileinfo {
    int length;
    char *content;
} fileinfo_t;

int run_server(void);
int get_fileinfo(char *path, fileinfo_t *curr_file);

#endif
