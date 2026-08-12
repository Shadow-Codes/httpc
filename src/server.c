#include "server.h"
#include "parser.h"
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define RECV_BUFFER_SIZE 1024
#define BODY_BUFFER_SIZE 1024
#define RESPONSE_BUFFER_SIZE (BODY_BUFFER_SIZE + 512)

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
                char body_buf[BODY_BUFFER_SIZE];
                char response_buf[RESPONSE_BUFFER_SIZE];
                // send
                snprintf(body_buf, BODY_BUFFER_SIZE,
                         "%s method request for %s received by host: %s.", current_request.method,
                         current_request.request_target, current_request.host);
                snprintf(
                    response_buf, RESPONSE_BUFFER_SIZE,
                    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
                    (int)strlen(body_buf), body_buf);

                int bytes_sent = send(new_fd, response_buf, strlen(response_buf), 0);
                if (bytes_sent == -1) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    exit(1);
                } else if (bytes_sent == (int)strlen(response_buf)) {
                    printf("Response sent: %d bytes\n", bytes_sent);
                }
            }
        }
        close(new_fd);
    }
    close(sock_fd);
    freeaddrinfo(servinfo);
    return 0;
}
