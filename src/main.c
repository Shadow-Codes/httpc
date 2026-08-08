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
#define BUFFER_SIZE 1024

int main(void) {
  printf("Getting started!\n");

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

  /*
  socket() -> bind() -> listen() -> accept() -> send() -> recv()
  */

  int sock_fd =
      socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
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
      char buf[BUFFER_SIZE];
      // receive
      int bytes_received = recv(new_fd, buf, BUFFER_SIZE, 0);
      if (bytes_received == -1) {
        fprintf(stderr, "receive: %s\n", strerror(errno));
        exit(1);
      } else if (bytes_received == 0) {
        printf("Client Disconnected\n");
        break;
      } else {
        buf[bytes_received] = '\0';
        printf("Received Message: %s", buf);
      }

      // echo back same data
      if (send(new_fd, buf, bytes_received, 0) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
      }
    }

    close(new_fd);
  }
  close(sock_fd);
  freeaddrinfo(servinfo);
  return 0;
}
