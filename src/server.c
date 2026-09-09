#include "server.h"

#include "io.h"
#include "router.h"
#include "request.h"
#include "response.h"
#include "file_store.h"

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

static int root_handler(
    const http_request_t *request,
    http_response_t *response
)
{
    (void)request;

    const char *body =
        "microshare server\n";

    if (http_response_set_status(
            response,
            200,
            "OK"
        ) == -1) {
        return -1;
    }

    if (http_response_set_content_type(
            response,
            "text/plain"
        ) == -1) {
        return -1;
    }

    http_response_set_body(
        response,
        body,
        strlen(body)
    );

    return 0;
}

static int files_handler(
    const http_request_t *request,
    http_response_t *response
)
{
    (void)request;

    enum {
        MAX_FILES = 128,
        RESPONSE_BUFFER_SIZE = 16384
    };

    file_entry_t entries[MAX_FILES];

    size_t entry_count = 0;

    if (file_store_list(
            "shared",
            entries,
            MAX_FILES,
            &entry_count
        ) == -1) {
        const char *error_body =
            "{\"error\":\"failed to list files\"}\n";

        if (http_response_set_status(
                response,
                500,
                "Internal Server Error"
            ) == -1) {
            return -1;
        }

        if (http_response_set_content_type(
                response,
                "application/json"
            ) == -1) {
            return -1;
        }

        http_response_set_body(
            response,
            error_body,
            strlen(error_body)
        );

        return 0;
    }

    static char body[RESPONSE_BUFFER_SIZE];

    size_t total_written = 0;

    int written = snprintf(
        body,
        sizeof(body),
        "{\n  \"files\": ["
    );

    if (
        written < 0 ||
        (size_t)written >= sizeof(body)
    ) {
        return -1;
    }

    total_written = (size_t)written;

    for (size_t i = 0; i < entry_count; i++) {
        const char *type = "unknown";

        if (entries[i].type == FILE_ENTRY_REGULAR) {
            type = "file";
        } else if (
            entries[i].type == FILE_ENTRY_DIRECTORY
        ) {
            type = "directory";
        }

        written = snprintf(
            body + total_written,
            sizeof(body) - total_written,
            "%s\n"
            "    {"
            "\"name\":\"%s\","
            "\"size\":%zu,"
            "\"type\":\"%s\""
            "}",
            i == 0 ? "" : ",",
            entries[i].name,
            entries[i].size,
            type
        );

        if (
            written < 0 ||
            (size_t)written >=
                sizeof(body) - total_written
        ) {
            return -1;
        }

        total_written += (size_t)written;
    }

    written = snprintf(
        body + total_written,
        sizeof(body) - total_written,
        "\n  ]\n}\n"
    );

    if (
        written < 0 ||
        (size_t)written >=
            sizeof(body) - total_written
    ) {
        return -1;
    }

    total_written += (size_t)written;

    if (http_response_set_status(
            response,
            200,
            "OK"
        ) == -1) {
        return -1;
    }

    if (http_response_set_content_type(
            response,
            "application/json"
        ) == -1) {
        return -1;
    }

    http_response_set_body(
        response,
        body,
        total_written
    );

    return 0;
}

static int file_download_handler(
    const http_request_t *request,
    http_response_t *response
)
{
    off_t file_size = 0;

    int file_fd = file_store_open(
        "shared",
        request->param,
        &file_size
    );

    if (file_fd == -1) {
        const char *body =
            "404 File Not Found\n";

        if (http_response_set_status(
                response,
                404,
                "Not Found"
            ) == -1) {
            return -1;
        }

        if (http_response_set_content_type(
                response,
                "text/plain"
            ) == -1) {
            return -1;
        }

        http_response_set_body(
            response,
            body,
            strlen(body)
        );

        return 0;
    }

    if (file_size < 0) {
        close(file_fd);
        return -1;
    }

    if (http_response_set_status(
            response,
            200,
            "OK"
        ) == -1) {
        close(file_fd);
        return -1;
    }

    if (http_response_set_content_type(
            response,
            "application/octet-stream"
        ) == -1) {
        close(file_fd);
        return -1;
    }

    http_response_set_file(
        response,
        file_fd,
        (size_t)file_size
    );

    return 0;
}

int http_server_run(
    http_server_t *server
)
{
    http_router_t router;
    http_router_init(&router);

    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/",
            root_handler
        ) == -1) {
        fprintf(stderr, "failed to register route /\n");
        return -1;
    }

    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/files",
            files_handler
        ) == -1) {
        fprintf(stderr, "failed to register route /files\n");
        return -1;
    }

    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/files/:filename",
            file_download_handler
        ) == -1) {
        fprintf(
            stderr,
            "failed to register route /files/:filename\n"
        );

        return -1;
    }

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

        int route_result = http_router_dispatch(
            &router,
            &request,
            &response
        );

        if (route_result == -1) {
            fprintf(
                stderr,
                "route handler failed\n"
            );

            close(client_fd);
            continue;
        }

        if (route_result == 1) {
            const char *body = "404 Not Found\n";

            if (http_response_set_status(
                    &response,
                    404,
                    "Not Found"
                ) == -1) {
                close(client_fd);
                continue;
            }

            if (http_response_set_content_type(
                    &response,
                    "text/plain"
                ) == -1) {
                close(client_fd);
                continue;
            }

            http_response_set_body(
                &response,
                body,
                strlen(body)
            );
        }

        if (http_response_send(
                client_fd,
                &response
            ) == -1) {
            perror("http_response_send");
        }

        if (
            response.body_type == HTTP_BODY_FILE &&
            response.file_fd != -1
        ) {
            close(response.file_fd);
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
