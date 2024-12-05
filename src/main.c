#include "http_server.h"
#include "config.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // Initialize logger
    if (logger_init("server.log") != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return EXIT_FAILURE;
    }

    INFO("HTTP Server starting on port %d...", PORT);
    DEBUG("Debug mode enabled");
    TRACE("Detailed logging activated");

    int result = initialize_server();

    logger_cleanup();
    return result;
}