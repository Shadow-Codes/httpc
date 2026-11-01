#include "server.h"
#include "http_error_codes.h"
#include "mime.h"
#include "parser.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define RECV_BUFFER_SIZE 8192

int run_server(void) {
    int setup_status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_storage their_info;
    struct sigaction sa;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((setup_status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(setup_status));
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

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        fprintf(stderr, "sigaction: %s\n", strerror(errno));
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {
        socklen_t addr_size = sizeof(their_info);
        int new_fd = accept(sock_fd, (struct sockaddr *)&their_info, &addr_size);
        if (new_fd == -1) {
            fprintf(stderr, "accept: %s\n", strerror(errno));
            exit(1);
        }

        if (!fork()) {
            close(sock_fd); // child process doesn't need listener

            // receive
            char recv_buf[RECV_BUFFER_SIZE] = "";
            int total = 0;
            while (1) {
                int bytes_received =
                    recv(new_fd, &recv_buf[total], RECV_BUFFER_SIZE - total - 1, 0);
                if (bytes_received == -1) {
                    fprintf(stderr, "receive: %s\n", strerror(errno));
                    close(new_fd);
                    exit(1);
                } else if (bytes_received == 0) {
                    printf("Client Disconnected\n");
                    close(new_fd);
                    exit(1);
                } else {
                    total += bytes_received;
                    recv_buf[total] = '\0';
                    if (strstr(recv_buf, "\r\n\r\n")) {
                        break;
                    } else if (total == RECV_BUFFER_SIZE - 1) {
                        send_error(new_fd, 431);
                        close(new_fd);
                        exit(1);
                    }
                }
            }

            http_request_t current_request;
            int status_code = parse_request(recv_buf, &current_request);
            if (status_code != 0) {
                send_error(new_fd, status_code == -1 ? 400 : status_code);
                fprintf(stderr, "Parser failed!\n");
                close(new_fd);
                exit(1);
            }
            fileinfo_t curr_file;
            size_t filepath_size = sizeof(current_request.request_target) + 8;
            char filepath[filepath_size];

            // call helper to build filepath as per request
            int filepath_status = build_filepath(&current_request, filepath, filepath_size);
            if (filepath_status != 0) {
                send_error(new_fd, filepath_status);
                close(new_fd);
                exit(1);
            }

            // send
            int file_status = get_fileinfo(filepath, &curr_file);
            if (file_status != 0) {
                send_error(new_fd, file_status);
                close(new_fd);
                exit(1);
            }
            char *response_buf = NULL;
            char *extension = malloc(16);
            if (extension == NULL) {
                fprintf(stderr, "Memory allocation for extension failed\n");
                close(new_fd);
                exit(1);
            }
            // extract extension for requested file
            get_extension(filepath, extension);
            send_valid_response(new_fd, &curr_file, extension);

            free(response_buf);
            free(extension);
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    close(sock_fd);
    freeaddrinfo(servinfo);
    return 0;
}

void sigchld_handler(int signum) {
    (void)signum;

    /*
    waitpid() runs asynchronously right inside parent process
    waitpid() call can silently overwrite errno
    */
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

int build_filepath(http_request_t *current_request, char *filepath, size_t filepath_size) {
    char *allowed_directory = "public/";
    char *abs_allowed_directory = realpath(allowed_directory, NULL);
    if (abs_allowed_directory == NULL) {
        if (errno == ENOENT) {
            return 404;
        } else {
            return 403;
        }
    }
    size_t abs_allowed_directory_len = strlen(abs_allowed_directory);

    if (strcmp(current_request->request_target, "/") == 0) {
        snprintf(filepath, filepath_size, "%s%s", allowed_directory, "index.html");
    } else {
        snprintf(filepath, filepath_size, "%s%s", allowed_directory,
                 current_request->request_target);
    }

    char *absolute_path = realpath(filepath, NULL);
    if (absolute_path == NULL) {
        if (errno == ENOENT) {
            free(abs_allowed_directory);
            return 404;
        } else {
            free(abs_allowed_directory);
            return 403;
        }
    }

    if (strncmp(absolute_path, abs_allowed_directory, abs_allowed_directory_len) != 0) {
        free(abs_allowed_directory);
        free(absolute_path);
        return 403;
    }

    if (absolute_path[abs_allowed_directory_len] != '/') {
        free(abs_allowed_directory);
        free(absolute_path);
        return 403;
    }

    free(abs_allowed_directory);
    free(absolute_path);
    return 0;
};

int get_fileinfo(char *filepath, fileinfo_t *curr_file) {
    FILE *read_ptr;

    read_ptr = fopen(filepath, "rb");
    if (read_ptr == NULL) {
        int err = errno;
        fprintf(stderr, "Error opening file for reading: %s\n", strerror(err));
        if (err == ENOENT)
            return 404;
        if (err == EACCES)
            return 403;
        return 500;
    }

    int fd = fileno(read_ptr);
    if (fd == -1) {
        fclose(read_ptr);
        fprintf(stderr, "Failed to get file discriptor: %s\n", strerror(errno));
        return 500;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == -1) {
        fclose(read_ptr);
        fprintf(stderr, "Error executing fstat: %s\n", strerror(errno));
        return 500;
    }

    curr_file->length = file_info.st_size;
    curr_file->file_handler = read_ptr;

    return 0;
}

void send_valid_response(int new_fd, fileinfo_t *curr_file, char *extension) {
    char response_headers[512];

    snprintf(response_headers, 512,
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: "
             "%zu\r\n\r\n",
             get_content_type(get_mimetype(extension)), curr_file->length);
    ssize_t send_status = send(new_fd, response_headers, strlen(response_headers), 0);

    if (send_status == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        fclose(curr_file->file_handler);
        close(new_fd);
        exit(1);
    } else if (send_status > 0) {
        printf("Response sent: %zd bytes\n", send_status);
    }

    printf("Sending Requested File...\n");
    char chunk_buf[8192];
    size_t file_sent = 0;
    while (file_sent < curr_file->length) {
        size_t chunk_len = fread(chunk_buf, 1, 8192, curr_file->file_handler);
        if (chunk_len == 0) {
            break;
        }
        size_t chunk_sent = 0;
        while (chunk_sent != chunk_len) {
            ssize_t chunk_status = send(new_fd, chunk_buf + chunk_sent, chunk_len - chunk_sent, 0);
            if (chunk_status == -1) {
                fprintf(stderr, "%s\n", strerror(errno));
                fclose(curr_file->file_handler);
                close(new_fd);
                exit(1);
            } else if (chunk_status > 0) {
                chunk_sent += chunk_status;
            }
        }
        file_sent += chunk_len;
    }

    if (file_sent != curr_file->length) {
        fprintf(stderr, "File sent is incomplete\nActual size: %zu\nSent size: %zu\n",
                curr_file->length, file_sent);
    }

    fclose(curr_file->file_handler);
}

void send_error(int new_fd, int code) {
    char response_buf[512];
    const char *phrase = get_reason_phrase(code);

    snprintf(response_buf, 512,
             "HTTP/1.1 %d %s\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: "
             "%zu\r\nConnection: "
             "close\r\n\r\n%s",
             code, phrase, strlen(phrase), phrase);

    send(new_fd, response_buf, strlen(response_buf), 0);
}

const char *get_reason_phrase(int code) {
    for (size_t i = 0; i < registery_len; i++) {
        if (code_registery[i].code == code) {
            return code_registery[i].phrase;
        }
    }
    return "Error";
}
