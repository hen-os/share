#ifndef MICROHTTPS_RESPONSE_H
#define MICROHTTPS_RESPONSE_H

#include <stddef.h>

#define HTTP_REASON_MAX_LENGTH 64
#define HTTP_CONTENT_TYPE_MAX_LENGTH 128

typedef enum {
    HTTP_BODY_NONE = 0,
    HTTP_BODY_MEMORY,
    HTTP_BODY_FILE
} http_body_type_t;

typedef struct {
    int status_code;

    char reason[HTTP_REASON_MAX_LENGTH];
    char content_type[HTTP_CONTENT_TYPE_MAX_LENGTH];

    http_body_type_t body_type;

    const void *body;
    size_t body_length;

    int file_fd;
} http_response_t;

void http_response_init(
    http_response_t *response
);

int http_response_set_headers(
    int client_fd,
    const http_response_t *response
);

int http_response_set_status(
    http_response_t *response,
    int status_code,
    const char *reason
);

int http_response_set_content_type(
    http_response_t *response,
    const char *content_type
);

void http_response_set_body(
    http_response_t *response,
    const void *body,
    size_t body_length
);

void http_response_set_file(
    http_response_t *response,
    int file_fd,
    size_t file_size
);

int http_response_send(
    int client_fd,
    const http_response_t *response
);

#endif
