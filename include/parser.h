#ifndef PARSER_H
#define PARSER_H

typedef struct http_request {
    char method[8];
    char request_target[256];
    char protocol[9];
    char host[256];
} http_request_t;

int parse_request(char *buf, http_request_t *req);
int parse_http_status_line(char *buf, http_request_t *req);
int parse_headers(char *start_of_headers, http_request_t *req);
int parse_header_line(char *cursor, http_request_t *req);

#endif
