#include "parser.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int parse_request(char *buf, http_request_t *req) {
    char line_delim = '\r';
    char *end_of_status_line = strchr(buf, line_delim);
    if (end_of_status_line == NULL) {
        return -1;
    }
    size_t length = end_of_status_line - buf;
    buf[length] = '\0';
    char *start_of_headers = end_of_status_line + 2;
    if (parse_http_status_line(buf, req) == -1) {
        return -1;
    };
    if (parse_headers(start_of_headers, req) == -1) {
        return -1;
    };

    return 0;
}

int parse_http_status_line(char *buf, http_request_t *req) {
    char struct_delim = ' ';
    char *method = strchr(buf, struct_delim);
    if (method == NULL) {
        return -1;
    }
    char *request_target = strchr(method + 1, struct_delim);
    if (request_target == NULL) {
        return -1;
    }
    char *protocol = strchr(request_target + 1, '\0');
    if (protocol == NULL) {
        return -1;
    }

    memcpy(req->method, buf, method - buf);
    req->method[method - buf] = '\0';
    memcpy(req->request_target, method + 1, request_target - (method + 1));
    req->request_target[request_target - (method + 1)] = '\0';
    memcpy(req->protocol, request_target + 1, protocol - (request_target + 1));
    req->protocol[protocol - (request_target + 1)] = '\0';
    return 0;
}

int parse_headers(char *start_of_headers, http_request_t *req) {
    char *cursor = start_of_headers;
    while (*cursor != '\r') {
        char *end_of_line = strchr(cursor, '\r');
        if (end_of_line == NULL) {
            return -1;
        }
        cursor[end_of_line - cursor] = '\0';
        if (parse_header_line(cursor, req) == -1) {
            return -1;
        };
        cursor = end_of_line + 2;
    }
    return 0;
}

int parse_header_line(char *cursor, http_request_t *req) {
    // parsing with : here
    char key[64];
    char *end_of_key = strchr(cursor, ':');
    if (end_of_key == NULL) {
        return -1;
    }
    memcpy(key, cursor, end_of_key - cursor);
    key[end_of_key - cursor] = '\0';

    if (strcmp(key, "Host") == 0) {
        char *end_of_value = strchr(cursor, '\0');
        if (end_of_value == NULL) {
            return -1;
        }
        memcpy(req->host, end_of_key + 2, end_of_value - (end_of_key + 2));
        req->host[end_of_value - (end_of_key + 2)] = '\0';
    }
    return 0;
}
