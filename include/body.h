#ifndef MICROHTTPS_BODY_H
#define MICROHTTPS_BODY_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
    int socket_fd;

    const unsigned char *initial_data;
    size_t initial_length;
    size_t initial_offset;

    size_t content_length;
    size_t total_read;
} http_body_stream_t;

void http_body_stream_init(
    http_body_stream_t *stream,
    int socket_fd,
    const unsigned char *initial_data,
    size_t initial_length,
    size_t content_length
);

ssize_t http_body_stream_read(
    http_body_stream_t *stream,
    void *buffer,
    size_t capacity
);

int http_body_stream_to_fd(
    http_body_stream_t *stream,
    int output_fd
);

#endif
