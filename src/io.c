#include "io.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

int send_all(
    int fd,
    const void *buffer,
    size_t length
)
{
    size_t total_sent = 0;

    const unsigned char *bytes = buffer;

    while (total_sent < length) {
        ssize_t bytes_sent = send(
            fd,
            bytes + total_sent,
            length - total_sent,
            0
        );

        if (bytes_sent == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes_sent == 0) {
            return -1;
        }

        total_sent += (size_t)bytes_sent;
    }

    return 0;
}

ssize_t recv_headers(
    int fd,
    char *buffer,
    size_t capacity
)
{
    size_t total_received = 0;

    while (total_received < capacity - 1) {
        ssize_t bytes_received = recv(
            fd,
            buffer + total_received,
            capacity - 1 - total_received,
            0
        );

        if (bytes_received == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes_received == 0) {
            break;
        }

        total_received += (size_t)bytes_received;

        buffer[total_received] = '\0';

        if (strstr(buffer, "\r\n\r\n") != NULL) {
            break;
        }
    }

    return (ssize_t)total_received;
}
