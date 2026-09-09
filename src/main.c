#include "server.h"

#include <stdlib.h>

#define PORT 8080
#define BACKLOG 16

int main(void)
{
    http_server_t server;

    if (http_server_init(
            &server,
            PORT,
            BACKLOG
        ) == -1) {
        return EXIT_FAILURE;
    }

    if (http_server_run(
            &server
        ) == -1) {
        http_server_close(&server);

        return EXIT_FAILURE;
    }

    http_server_close(&server);

    return EXIT_SUCCESS;
}
