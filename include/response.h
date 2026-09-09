#ifndef MICROHTTPS_RESPONSE_H
#define MICROHTTPS_RESPONSE_H

#include <stddef.h>

#define HTTP_REASON_MAX_LENGTH 64
#define HTTP_CONTENT_TYPE_MAX_LENGTH 128

typedef struct {
    int status_code;

    char reason[HTTP_REASON_MAX_LENGTH];
    char content_type[HTTP_CONTENT_TYPE_MAX_LENGTH];

    const void *body;
    size_t body_length;
} http_response_t;

void http_response_init(
    http_response_t *response
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

int http_response_send(
    int client_fd,
    const http_response_t *response
);

#endif
