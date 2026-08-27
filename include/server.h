#ifndef SERVER_H
#define SERVER_H

#include "parser.h"
#include <stddef.h>

typedef struct fileinfo {
    int length;
    char *content;
} fileinfo_t;

int run_server(void);
void build_filepath(http_request_t *curr_file, char *filepath, size_t filepath_size);
int get_fileinfo(char *path, fileinfo_t *curr_file);
void build_response(int file_status, fileinfo_t *curr_file, char *extension, char **response_buf);

#endif
