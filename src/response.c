#include "response.h"

#include "io.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define RESPONSE_HEADER_BUFFER_SIZE 1024
#define FILE_STREAM_BUFFER_SIZE (64 * 1024)

void http_response_init(
    http_response_t *response
)
{
    response->status_code = 200;

    strcpy(
        response->reason,
        "OK"
    );

    strcpy(
        response->content_type,
        "application/octet-stream"
    );

    response->body_type = HTTP_BODY_NONE;
    response->body = NULL;
    response->body_length = 0;
    response->file_fd = -1;
}

int http_response_set_headers(
    int client_fd,
    const http_response_t *response
)
{
    char header[RESPONSE_HEADER_BUFFER_SIZE];

    int header_length = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        response->status_code,
        response->reason,
        response->content_type,
        response->body_length
    );

    if (
        header_length < 0 ||
        (size_t)header_length >= sizeof(header)
    ) {
        return -1;
    }

    return send_all(
        client_fd,
        header,
        (size_t)header_length
    );
}

int http_response_set_status(
    http_response_t *response,
    int status_code,
    const char *reason
)
{
    size_t length = strlen(reason);

    if (length >= sizeof(response->reason)) {
        return -1;
    }

    response->status_code = status_code;

    memcpy(
        response->reason,
        reason,
        length + 1
    );

    return 0;
}

int http_response_set_content_type(
    http_response_t *response,
    const char *content_type
)
{
    size_t length = strlen(content_type);

    if (length >= sizeof(response->content_type)) {
        return -1;
    }

    memcpy(
        response->content_type,
        content_type,
        length + 1
    );

    return 0;
}

void http_response_set_body(
    http_response_t *response,
    const void *body,
    size_t body_length
)
{
    response->body_type = HTTP_BODY_MEMORY;
    response->body = body;
    response->body_length = body_length;
}

int http_response_send(
    int client_fd,
    const http_response_t *response
)
{
    if (http_response_set_headers(
            client_fd,
            response
        ) == -1) {
        return -1;
    }

    if (response->body_type == HTTP_BODY_MEMORY) {
        if (
            response->body != NULL &&
            response->body_length > 0
        ) {
            if (send_all(
                    client_fd,
                    response->body,
                    response->body_length
                ) == -1) {
                return -1;
            }
        }

        return 0;
    }

    if (response->body_type == HTTP_BODY_FILE) {
        unsigned char buffer[
            FILE_STREAM_BUFFER_SIZE
        ];

        for (;;) {
            ssize_t bytes_read = read(
                response->file_fd,
                buffer,
                sizeof(buffer)
            );

            if (bytes_read == -1) {
                if (errno == EINTR) {
                    continue;
                }

                return -1;
            }

            if (bytes_read == 0) {
                break;
            }

            if (send_all(
                    client_fd,
                    buffer,
                    (size_t)bytes_read
                ) == -1) {
                return -1;
            }
        }
    }

    return 0;
}

void http_response_set_file(
    http_response_t *response,
    int file_fd,
    size_t file_size
)
{
    response->body_type = HTTP_BODY_FILE;
    response->body = NULL;
    response->body_length = file_size;
    response->file_fd = file_fd;
}
