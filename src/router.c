#include "router.h"

#include <string.h>

void http_router_init(
    http_router_t *router
)
{
    memset(
        router,
        0,
        sizeof(*router)
    );
}

int http_router_add(
    http_router_t *router,
    http_method_t method,
    const char *path,
    http_handler_t handler
)
{
    if (router->route_count >= HTTP_ROUTER_MAX_ROUTES) {
        return -1;
    }

    size_t path_length = strlen(path);

    if (path_length >= HTTP_ROUTE_PATH_MAX_LENGTH) {
        return -1;
    }

    http_route_t *route =
        &router->routes[router->route_count];

    route->method = method;
    route->handler = handler;

    memcpy(
        route->path,
        path,
        path_length + 1
    );

    router->route_count++;

    return 0;
}

static int match_route(
    const char *pattern,
    const char *path,
    char *param,
    size_t param_capacity
)
{
    const char *parameter = strchr(pattern, ':');

    if (parameter == NULL) {
        return strcmp(pattern, path) == 0;
    }

    size_t prefix_length =
        (size_t)(parameter - pattern);

    if (strncmp(
            pattern,
            path,
            prefix_length
        ) != 0) {
        return 0;
    }

    const char *value =
        path + prefix_length;

    if (*value == '\0') {
        return 0;
    }

    size_t value_length = strlen(value);

    if (value_length >= param_capacity) {
        return 0;
    }

    memcpy(
        param,
        value,
        value_length + 1
    );

    return 1;
}

int http_router_dispatch(
    const http_router_t *router,
    http_request_t *request,
    http_body_stream_t *body,
    http_response_t *response
)
{
    request->param[0] = '\0';

    for (size_t i = 0; i < router->route_count; i++) {
        const http_route_t *route =
            &router->routes[i];

        if (route->method != request->method) {
            continue;
        }

        if (!match_route(
                route->path,
                request->path,
                request->param,
                sizeof(request->param)
            )) {
            continue;
        }

        return route->handler(
            request,
            body,
            response
        );
    }

    return 1;
}
