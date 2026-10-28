#ifndef SERVER_H
#define SERVER_H

#include "parser.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct fileinfo {
    size_t length;
    FILE *file_handler;
} fileinfo_t;

int run_server(void);
void sigchld_handler(int signum);
void build_filepath(http_request_t *curr_file, char *filepath, size_t filepath_size);
int get_fileinfo(char *path, fileinfo_t *curr_file);
void send_valid_response(int new_fd, fileinfo_t *curr_file, char *file_extension);
void send_error(int new_fd, int code);
const char *get_reason_phrase(int code);

#endif
