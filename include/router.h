#ifndef MICROHTTPS_ROUTER_H
#define MICROHTTPS_ROUTER_H

#include "http.h"
#include "request.h"
#include "response.h"

#include <stddef.h>

#define HTTP_ROUTER_MAX_ROUTES 32
#define HTTP_ROUTE_PATH_MAX_LENGTH 2048

typedef int (*http_handler_t)(
    const http_request_t *request,
    http_response_t *response
);

typedef struct {
    http_method_t method;

    char path[HTTP_ROUTE_PATH_MAX_LENGTH];

    http_handler_t handler;
} http_route_t;

typedef struct {
    http_route_t routes[HTTP_ROUTER_MAX_ROUTES];

    size_t route_count;
} http_router_t;

void http_router_init(
    http_router_t *router
);

int http_router_add(
    http_router_t *router,
    http_method_t method,
    const char *path,
    http_handler_t handler
);

int http_router_dispatch(
    const http_router_t *router,
    http_request_t *request,
    http_response_t *response
);

#endif
