#ifndef MICROHTTPS_REQUEST_H
#define MICROHTTPS_REQUEST_H

#include "http.h"

#define HTTP_METHOD_MAX_LENGTH 16
#define HTTP_PATH_MAX_LENGTH 2048
#define HTTP_VERSION_MAX_LENGTH 16
#define HTTP_PARAM_MAX_LENGTH 2048

typedef struct {
    http_method_t method;

    char method_raw[HTTP_METHOD_MAX_LENGTH];
    char path[HTTP_PATH_MAX_LENGTH];
    char version[HTTP_VERSION_MAX_LENGTH];

    char param[HTTP_PARAM_MAX_LENGTH];
} http_request_t;

int http_request_parse(
    const char *buffer,
    http_request_t *request
);

#endif
