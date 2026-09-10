#ifndef MICROHTTPS_IO_H
#define MICROHTTPS_IO_H

#include <stddef.h>
#include <sys/types.h>

int send_all(
    int fd,
    const void *buffer,
    size_t length
);

int write_all(
    int fd,
    const void *buffer,
    size_t length
);

ssize_t recv_headers(
    int fd,
    char *buffer,
    size_t capacity
);

int recv_to_file(
    int socket_fd,
    int file_fd,
    const unsigned char *initial_body,
    size_t initial_length,
    size_t total_length
);

#endif
