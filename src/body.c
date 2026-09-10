#include "body.h"

#include "io.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

void http_body_stream_init(
    http_body_stream_t *stream,
    int socket_fd,
    const unsigned char *initial_data,
    size_t initial_length,
    size_t content_length
)
{
    stream->socket_fd = socket_fd;

    stream->initial_data = initial_data;
    stream->initial_length = initial_length;
    stream->initial_offset = 0;

    stream->content_length = content_length;
    stream->total_read = 0;
}

ssize_t http_body_stream_read(
    http_body_stream_t *stream,
    void *buffer,
    size_t capacity
)
{
    if (stream->total_read >= stream->content_length) {
        return 0;
    }

    size_t remaining =
        stream->content_length - stream->total_read;

    if (capacity > remaining) {
        capacity = remaining;
    }

    if (stream->initial_offset < stream->initial_length) {
        size_t initial_remaining =
            stream->initial_length -
            stream->initial_offset;

        size_t copy_length =
            initial_remaining < capacity
                ? initial_remaining
                : capacity;

        memcpy(
            buffer,
            stream->initial_data +
                stream->initial_offset,
            copy_length
        );

        stream->initial_offset += copy_length;
        stream->total_read += copy_length;

        return (ssize_t)copy_length;
    }

    for (;;) {
        ssize_t received = recv(
            stream->socket_fd,
            buffer,
            capacity,
            0
        );

        if (received == -1) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (received == 0) {
            return -1;
        }

        stream->total_read +=
            (size_t)received;

        return received;
    }
}

int http_body_stream_to_fd(
    http_body_stream_t *stream,
    int output_fd
)
{
    unsigned char buffer[64 * 1024];

    while (stream->total_read < stream->content_length) {
        ssize_t bytes_read =
            http_body_stream_read(
                stream,
                buffer,
                sizeof(buffer)
            );

        if (bytes_read <= 0) {
            return -1;
        }

        if (write_all(
                output_fd,
                buffer,
                (size_t)bytes_read
            ) == -1) {
            return -1;
        }
    }

    return 0;
}
