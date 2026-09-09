#include "request.h"

#include <stdio.h>
#include <string.h>

static http_method_t parse_method(
    const char *method
)
{
    if (strcmp(method, "GET") == 0) {
        return HTTP_METHOD_GET;
    }

    if (strcmp(method, "POST") == 0) {
        return HTTP_METHOD_POST;
    }

    if (strcmp(method, "PUT") == 0) {
        return HTTP_METHOD_PUT;
    }

    if (strcmp(method, "PATCH") == 0) {
        return HTTP_METHOD_PATCH;
    }

    if (strcmp(method, "DELETE") == 0) {
        return HTTP_METHOD_DELETE;
    }

    if (strcmp(method, "HEAD") == 0) {
        return HTTP_METHOD_HEAD;
    }

    if (strcmp(method, "OPTIONS") == 0) {
        return HTTP_METHOD_OPTIONS;
    }

    return HTTP_METHOD_UNKNOWN;
}

int http_request_parse(
    const char *buffer,
    http_request_t *request
)
{
    int parsed;

    memset(
        request,
        0,
        sizeof(*request)
    );

    parsed = sscanf(
        buffer,
        "%15s %2047s %15s",
        request->method_raw,
        request->path,
        request->version
    );

    if (parsed != 3) {
        return -1;
    }

    request->method = parse_method(
        request->method_raw
    );

    return 0;
}
