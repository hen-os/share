#ifndef MICROHTTPS_IO_H
#define MICROHTTPS_IO_H

#include <stddef.h>
#include <sys/types.h>

int send_all(
    int fd,
    const void *buffer,
    size_t length
);

ssize_t recv_headers(
    int fd,
    char *buffer,
    size_t capacity
);

#endif
