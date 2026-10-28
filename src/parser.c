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
    char *start_of_headers = end_of_status_line + 2;
    int status_code = parse_http_status_line(buf, req, length);
    if (status_code != 0) {
        return status_code;
    };
    if (parse_headers(start_of_headers, req) == -1) {
        return -1;
    };

    return 0;
}

int parse_http_status_line(char *buf, http_request_t *req, size_t length) {
    char struct_delim = ' ';
    char *method = strchr(buf, struct_delim);
    if (method == NULL) {
        return -1;
    }
    char *request_target = strchr(method + 1, struct_delim);
    if (request_target == NULL || request_target >= buf + length) {
        return -1;
    }
    char *protocol = buf + length;

    size_t method_len = (size_t)(method - buf);
    size_t request_target_len = (size_t)(request_target - (method + 1));
    size_t protocol_len = (size_t)(protocol - (request_target + 1));

    if (method_len < sizeof(req->method)) {
        memcpy(req->method, buf, method - buf);
        req->method[method - buf] = '\0';
    } else {
        return -1;
    }
    if (request_target_len < sizeof(req->request_target)) {
        memcpy(req->request_target, method + 1, request_target - (method + 1));
        req->request_target[request_target - (method + 1)] = '\0';
    } else {
        return -1;
    }

    if (protocol_len < sizeof(req->protocol)) {
        memcpy(req->protocol, request_target + 1, protocol - (request_target + 1));
        req->protocol[protocol - (request_target + 1)] = '\0';
    } else {
        return -1;
    }

    // check protocol version is either 1.0 or 1.1
    if ((strcmp(req->protocol, "HTTP/1.1") != 0) && (strcmp(req->protocol, "HTTP/1.0") != 0)) {
        return 505;
    }

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
        cursor = end_of_line + 2; // skip CRLF and move to start of next line
    }
    return 0;
}

/*
Divides Key->Value pairs with : as delimiter
Checks if Key or Value fits in their respective field sizes
If Key is Host, then adds Value to struct
*/
int parse_header_line(char *cursor, http_request_t *req) {
    // parsing with : here
    char key[64];
    char *end_of_key = strchr(cursor, ':');
    if (end_of_key == NULL) {
        return -1;
    }

    if ((size_t)(end_of_key - cursor) < sizeof(key)) {
        memcpy(key, cursor, end_of_key - cursor);
        key[end_of_key - cursor] = '\0';
    } else {
        return -1;
    }

    if (strcmp(key, "Host") == 0) {
        char *end_of_value = strchr(cursor, '\0');
        char *start_of_value = end_of_key + 1;
        while (*start_of_value == ' ') {
            start_of_value++;
        }

        size_t len_of_value = (size_t)(end_of_value - start_of_value);
        if (len_of_value < sizeof(req->host)) {
            memcpy(req->host, start_of_value, end_of_value - start_of_value);
            req->host[end_of_value - start_of_value] = '\0';
        } else {
            return -1;
        }
    }
    return 0;
}
