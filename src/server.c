#include "server.h"

#include "body.h"
#include "file_store.h"
#include "io.h"
#include "request.h"
#include "response.h"
#include "router.h"

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


/*
 * GET /
 */
static int root_handler(
    const http_request_t *request,
    http_body_stream_t *body_stream,
    http_response_t *response
)
{
    (void)request;
    (void)body_stream;

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


/*
 * GET /files
 */
static int files_handler(
    const http_request_t *request,
    http_body_stream_t *body_stream,
    http_response_t *response
)
{
    (void)request;
    (void)body_stream;

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
        "{\n"
        "  \"files\": ["
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

        total_written +=
            (size_t)written;
    }

    written = snprintf(
        body + total_written,
        sizeof(body) - total_written,
        "\n"
        "  ]\n"
        "}\n"
    );

    if (
        written < 0 ||
        (size_t)written >=
            sizeof(body) - total_written
    ) {
        return -1;
    }

    total_written +=
        (size_t)written;

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


/*
 * POST /files/:filename
 */
static int file_upload_handler(
    const http_request_t *request,
    http_body_stream_t *body_stream,
    http_response_t *response
)
{
    char temp_path[
        FILE_STORE_PATH_MAX_LENGTH
    ];

    int upload_fd =
        file_store_create_temp(
            "shared",
            request->param,
            temp_path,
            sizeof(temp_path)
        );

    if (upload_fd == -1) {
        const char *response_body =
            "invalid filename or upload error\n";

        if (http_response_set_status(
                response,
                400,
                "Bad Request"
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
            response_body,
            strlen(response_body)
        );

        return 0;
    }

    /*
     * Consome o body HTTP inteiro e escreve
     * diretamente no arquivo temporário.
     */
    if (http_body_stream_to_fd(
            body_stream,
            upload_fd
        ) == -1) {
        close(upload_fd);

        file_store_remove(
            temp_path
        );

        const char *response_body =
            "incomplete upload\n";

        if (http_response_set_status(
                response,
                400,
                "Bad Request"
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
            response_body,
            strlen(response_body)
        );

        return 0;
    }

    /*
     * Garante que os dados sejam descarregados
     * para o filesystem antes do commit.
     */
    if (fsync(upload_fd) == -1) {
        close(upload_fd);

        file_store_remove(
            temp_path
        );

        return -1;
    }

    if (close(upload_fd) == -1) {
        file_store_remove(
            temp_path
        );

        return -1;
    }

    /*
     * Renomeia o arquivo temporário para
     * seu nome definitivo.
     */
    if (file_store_commit(
            temp_path,
            "shared",
            request->param
        ) == -1) {
        file_store_remove(
            temp_path
        );

        const char *response_body =
            "failed to commit upload\n";

        if (http_response_set_status(
                response,
                500,
                "Internal Server Error"
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
            response_body,
            strlen(response_body)
        );

        return 0;
    }

    const char *response_body =
        "file uploaded\n";

    if (http_response_set_status(
            response,
            201,
            "Created"
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
        response_body,
        strlen(response_body)
    );

    return 0;
}


/*
 * GET /files/:filename
 */
static int file_download_handler(
    const http_request_t *request,
    http_body_stream_t *body_stream,
    http_response_t *response
)
{
    (void)body_stream;

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

    http_router_init(
        &router
    );

    /*
     * GET /
     */
    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/",
            root_handler
        ) == -1) {
        fprintf(
            stderr,
            "failed to register route /\n"
        );

        return -1;
    }

    /*
     * GET /files
     */
    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/files",
            files_handler
        ) == -1) {
        fprintf(
            stderr,
            "failed to register route /files\n"
        );

        return -1;
    }

    /*
     * GET /files/:filename
     */
    if (http_router_add(
            &router,
            HTTP_METHOD_GET,
            "/files/:filename",
            file_download_handler
        ) == -1) {
        fprintf(
            stderr,
            "failed to register route "
            "/files/:filename\n"
        );

        return -1;
    }

    /*
     * POST /files/:filename
     */
    if (http_router_add(
            &router,
            HTTP_METHOD_POST,
            "/files/:filename",
            file_upload_handler
        ) == -1) {
        fprintf(
            stderr,
            "failed to register POST "
            "/files/:filename\n"
        );

        return -1;
    }

    char buffer[BUFFER_SIZE];

    printf(
        "microhttps listening on "
        "0.0.0.0:%u\n",
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

        ssize_t bytes_received =
            recv_headers(
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

        /*
         * Faz o parsing da request HTTP.
         */
        http_request_t request;

        if (http_request_parse(
                buffer,
                (size_t)bytes_received,
                &request
            ) == -1) {
            fprintf(
                stderr,
                "invalid HTTP request\n"
            );

            close(client_fd);

            continue;
        }

        /*
         * Cria a response inicialmente vazia.
         */
        http_response_t response;

        http_response_init(
            &response
        );

        /*
         * Cria uma abstração sobre o body.
         *
         * Ela primeiro consumirá os bytes que
         * já vieram junto com os headers e,
         * depois, continuará lendo do socket.
         */
        http_body_stream_t body_stream;

        http_body_stream_init(
            &body_stream,
            client_fd,
            request.body,
            request.body_received,
            request.content_length
        );

        /*
         * O router decide qual handler executar.
         */
        int route_result =
            http_router_dispatch(
                &router,
                &request,
                &body_stream,
                &response
            );

        /*
         * Handler falhou internamente.
         */
        if (route_result == -1) {
            fprintf(
                stderr,
                "route handler failed\n"
            );

            close(client_fd);

            continue;
        }

        /*
         * Nenhuma rota encontrada.
         */
        if (route_result == 1) {
            const char *body =
                "404 Not Found\n";

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

        /*
         * Serializa headers e envia body.
         *
         * HTTP_BODY_MEMORY:
         *     envia memória.
         *
         * HTTP_BODY_FILE:
         *     faz streaming do arquivo.
         */
        if (http_response_send(
                client_fd,
                &response
            ) == -1) {
            perror(
                "http_response_send"
            );
        }

        /*
         * Se a response abriu um arquivo para
         * download, o descriptor precisa ser
         * fechado depois do streaming.
         */
        if (
            response.body_type ==
                HTTP_BODY_FILE &&
            response.file_fd != -1
        ) {
            close(
                response.file_fd
            );
        }

        printf(
            "\n"
            "--- request ---\n"
            "method:          %s\n"
            "path:            %s\n"
            "version:         %s\n"
            "content-length:  %zu\n"
            "body-received:   %zu\n"
            "---------------\n",
            request.method_raw,
            request.path,
            request.version,
            request.content_length,
            request.body_received
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
        close(
            server->fd
        );

        server->fd = -1;
    }
}
