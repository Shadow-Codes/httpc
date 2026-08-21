#include "server.h"
#include "mime.h"
#include "parser.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define RECV_BUFFER_SIZE 1024

int run_server(void) {
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_storage their_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    int sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sock_fd == -1) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        exit(1);
    }

    if (bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        exit(1);
    }

    if (listen(sock_fd, BACKLOG) == -1) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        exit(1);
    }

    while (1) {
        socklen_t addr_size = sizeof(their_info);
        int new_fd = accept(sock_fd, (struct sockaddr *)&their_info, &addr_size);
        if (new_fd == -1) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            exit(1);
        }

        while (1) {
            char recv_buf[RECV_BUFFER_SIZE];
            // receive
            int bytes_received = recv(new_fd, recv_buf, RECV_BUFFER_SIZE, 0);
            if (bytes_received == -1) {
                fprintf(stderr, "receive: %s\n", strerror(errno));
                exit(1);
            } else if (bytes_received == 0) {
                printf("Client Disconnected\n");
                break;
            } else {
                recv_buf[bytes_received] = '\0';
                http_request_t current_request;
                if (parse_request(recv_buf, &current_request) == -1) {
                    fprintf(stderr, "Parser failed!\n");
                    continue;
                }
                fileinfo_t curr_file;
                char filepath[sizeof(current_request.request_target) + 8];
                // send
                if (strcmp(current_request.request_target, "/") == 0) {
                    snprintf(filepath, sizeof(filepath), "public/%s", "index.html");
                } else {
                    snprintf(filepath, sizeof(filepath), "public%s",
                             current_request.request_target);
                }

                int status = get_fileinfo(filepath, &curr_file);
                char *response_buf = NULL;
                char *extension = malloc(16);
                if (extension == NULL) {
                    fprintf(stderr, "Memory allocation for extension failed\n");
                    continue;
                }
                // extract extension for requested file
                get_extension(filepath, extension);

                if (status == -1) {
                    response_buf = malloc(512);
                    if (response_buf == NULL) {
                        fprintf(stderr, "Dynamic memory allocation for HTTP response failed!\n");
                        continue;
                    }
                    snprintf(response_buf, 512,
                             "HTTP/1.1 404 Not Found\r\nContent-Type: "
                             "text/plain\r\nContent-Length: "
                             "10\r\n\r\n%s",
                             "Not Found");
                } else {
                    response_buf = malloc(curr_file.length + 512);
                    if (response_buf == NULL) {
                        fprintf(stderr, "Dynamic memory allocation for HTTP response failed!\n");
                        free(curr_file.content);
                        continue;
                    }
                    snprintf(response_buf, curr_file.length + 512,
                             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: "
                             "%d\r\n\r\n%s",
                             get_content_type(get_mimetype(extension)), curr_file.length,
                             curr_file.content);
                    free(curr_file.content);
                }
                int bytes_sent = send(new_fd, response_buf, strlen(response_buf), 0);
                if (bytes_sent == -1) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    exit(1);
                } else if (bytes_sent == (int)strlen(response_buf)) {
                    printf("Response sent: %d bytes\n", bytes_sent);
                }
                free(response_buf);
                free(extension);
            }
        }
        close(new_fd);
    }
    close(sock_fd);
    freeaddrinfo(servinfo);
    return 0;
}

int get_fileinfo(char *filepath, fileinfo_t *curr_file) {
    FILE *read_ptr;

    read_ptr = fopen(filepath, "r");
    if (read_ptr == NULL) {
        fprintf(stderr, "Error opening file for reading: %s\n", strerror(errno));
        return -1;
    }

    int fd = fileno(read_ptr);
    if (fd == -1) {
        fclose(read_ptr);
        fprintf(stderr, "Failed to get file discriptor: %s\n", strerror(errno));
        return -1;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == -1) {
        fclose(read_ptr);
        fprintf(stderr, "Error executing fstat: %s\n", strerror(errno));
        return -1;
    }

    char *buf = malloc(file_info.st_size + 1);
    if (buf == NULL) {
        fclose(read_ptr);
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    fread(buf, 1, file_info.st_size, read_ptr);
    buf[file_info.st_size] = '\0';

    curr_file->length = (int)file_info.st_size;
    curr_file->content = buf;

    fclose(read_ptr);
    return 0;
}
