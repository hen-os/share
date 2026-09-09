#ifndef MICROHTTPS_SERVER_H
#define MICROHTTPS_SERVER_H

#include <stdint.h>

typedef struct {
    int fd;
    uint16_t port;
    int backlog;
} http_server_t;

int http_server_init(
    http_server_t *server,
    uint16_t port,
    int backlog
);

int http_server_run(
    http_server_t *server
);

void http_server_close(
    http_server_t *server
);

#endif
