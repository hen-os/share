#include "server.h"

#include "io.h"
#include "request.h"
#include "response.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int http_server_init(
    http_server_t *server,
    uint16_t port,
    int backlog
)
{
    struct sockaddr_in server_addr;

    int reuse = 1;

    memset(
        server,
        0,
        sizeof(*server)
    );

    server->fd = -1;
    server->port = port;
    server->backlog = backlog;

    server->fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server->fd == -1) {
        perror("socket");
        return -1;
    }

    if (setsockopt(
            server->fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)
        ) == -1) {
        perror("setsockopt");

        http_server_close(server);

        return -1;
    }

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =
        htonl(INADDR_ANY);

    server_addr.sin_port =
        htons(port);

    if (bind(
            server->fd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)
        ) == -1) {
        perror("bind");

        http_server_close(server);

        return -1;
    }

    if (listen(
            server->fd,
            backlog
        ) == -1) {
        perror("listen");

        http_server_close(server);

        return -1;
    }

    return 0;
}

int http_server_run(
    http_server_t *server
)
{
    char buffer[BUFFER_SIZE];

    printf(
        "microhttps listening on 0.0.0.0:%u\n",
        (unsigned int)server->port
    );

    for (;;) {
        struct sockaddr_in client_addr;

        socklen_t client_addr_len =
            sizeof(client_addr);

        int client_fd = accept(
            server->fd,
            (struct sockaddr *)&client_addr,
            &client_addr_len
        );

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        ssize_t bytes_received = recv_headers(
            client_fd,
            buffer,
            sizeof(buffer)
        );

        if (bytes_received == -1) {
            perror("recv_headers");

            close(client_fd);
            continue;
        }

        if (bytes_received == 0) {
            close(client_fd);
            continue;
        }

        http_request_t request;

        if (http_request_parse(
                buffer,
                &request
            ) == -1) {
            fprintf(
                stderr,
                "invalid HTTP request\n"
            );

            close(client_fd);
            continue;
        }

        http_response_t response;

        http_response_init(&response);

        const char *body = "Hello, world!";

        http_response_set_status(
            &response,
            200,
            "OK"
        );

        http_response_set_content_type(
            &response,
            "text/plain"
        );

        http_response_set_body(
            &response,
            body,
            strlen(body)
        );

        if (http_response_send(
                client_fd,
                &response
            ) == -1) {
            perror("http_response_send");
        }

        printf(
            "\n--- request ---\n"
            "method:  %s\n"
            "path:    %s\n"
            "version: %s\n"
            "---------------\n",
            request.method_raw,
            request.path,
            request.version
        );

        close(client_fd);
    }

    return 0;
}

void http_server_close(
    http_server_t *server
)
{
    if (server->fd != -1) {
        close(server->fd);
        server->fd = -1;
    }
}
