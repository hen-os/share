#include "request.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

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

static const char *find_header_end(
    const char *buffer,
    size_t length
)
{
    if (length < 4) {
        return NULL;
    }

    for (size_t i = 0; i <= length - 4; i++) {
        if (
            buffer[i] == '\r' &&
            buffer[i + 1] == '\n' &&
            buffer[i + 2] == '\r' &&
            buffer[i + 3] == '\n'
        ) {
            return buffer + i;
        }
    }

    return NULL;
}

static int parse_request_line(
    const char *buffer,
    const char *line_end,
    http_request_t *request
)
{
    const char *first_space = NULL;
    const char *second_space = NULL;

    for (const char *p = buffer; p < line_end; p++) {
        if (*p == ' ') {
            if (first_space == NULL) {
                first_space = p;
            } else {
                second_space = p;
                break;
            }
        }
    }

    if (
        first_space == NULL ||
        second_space == NULL
    ) {
        return -1;
    }

    size_t method_length =
        (size_t)(first_space - buffer);

    size_t path_length =
        (size_t)(second_space - first_space - 1);

    size_t version_length =
        (size_t)(line_end - second_space - 1);

    if (
        method_length == 0 ||
        method_length >= sizeof(request->method_raw) ||
        path_length == 0 ||
        path_length >= sizeof(request->path) ||
        version_length == 0 ||
        version_length >= sizeof(request->version)
    ) {
        return -1;
    }

    memcpy(
        request->method_raw,
        buffer,
        method_length
    );

    request->method_raw[method_length] = '\0';

    memcpy(
        request->path,
        first_space + 1,
        path_length
    );

    request->path[path_length] = '\0';

    memcpy(
        request->version,
        second_space + 1,
        version_length
    );

    request->version[version_length] = '\0';

    request->method =
        parse_method(request->method_raw);

    return 0;
}

static int parse_content_length(
    const char *headers_begin,
    const char *headers_end,
    size_t *content_length
)
{
    const char *line = headers_begin;

    *content_length = 0;

    while (line < headers_end) {
        const char *line_end = line;

        while (
            line_end < headers_end &&
            !(line_end[0] == '\r' &&
              line_end + 1 < headers_end &&
              line_end[1] == '\n')
        ) {
            line_end++;
        }

        static const char header_name[] =
            "Content-Length:";

        size_t header_name_length =
            sizeof(header_name) - 1;

        size_t line_length =
            (size_t)(line_end - line);

        if (
            line_length >= header_name_length &&
            strncasecmp(
                line,
                header_name,
                header_name_length
            ) == 0
        ) {
            const char *value =
                line + header_name_length;

            while (
                value < line_end &&
                (*value == ' ' || *value == '\t')
            ) {
                value++;
            }

            if (value == line_end) {
                return -1;
            }

            char number[32];

            size_t value_length =
                (size_t)(line_end - value);

            if (value_length >= sizeof(number)) {
                return -1;
            }

            memcpy(
                number,
                value,
                value_length
            );

            number[value_length] = '\0';

            errno = 0;

            char *end = NULL;

            unsigned long long parsed =
                strtoull(
                    number,
                    &end,
                    10
                );

            if (
                errno != 0 ||
                end == number ||
                *end != '\0'
            ) {
                return -1;
            }

            if (parsed > SIZE_MAX) {
                return -1;
            }

            *content_length = (size_t)parsed;

            return 0;
        }

        if (line_end >= headers_end) {
            break;
        }

        line = line_end + 2;
    }

    return 0;
}

int http_request_parse(
    const char *buffer,
    size_t buffer_length,
    http_request_t *request
)
{
    memset(
        request,
        0,
        sizeof(*request)
    );

    const char *header_end =
        find_header_end(
            buffer,
            buffer_length
        );

    if (header_end == NULL) {
        return -1;
    }

    const char *request_line_end = NULL;

    for (
        const char *p = buffer;
        p + 1 < header_end;
        p++
    ) {
        if (
            p[0] == '\r' &&
            p[1] == '\n'
        ) {
            request_line_end = p;
            break;
        }
    }

    if (request_line_end == NULL) {
        return -1;
    }

    if (parse_request_line(
            buffer,
            request_line_end,
            request
        ) == -1) {
        return -1;
    }

    const char *headers_begin =
        request_line_end + 2;

    if (parse_content_length(
            headers_begin,
            header_end,
            &request->content_length
        ) == -1) {
        return -1;
    }

    const unsigned char *body_begin =
        (const unsigned char *)header_end + 4;

    const unsigned char *buffer_end =
        (const unsigned char *)buffer +
        buffer_length;

    request->body = body_begin;

    request->body_received =
        (size_t)(buffer_end - body_begin);

    /*
     * Não podemos considerar como body mais bytes
     * do que Content-Length declarou.
     */
    if (
        request->body_received >
        request->content_length
    ) {
        request->body_received =
            request->content_length;
    }

    return 0;
}
