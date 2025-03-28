/**
 * @file main.cpp
 * @brief Main file for router application
 */

#include <csignal>
#include <iostream>
#include <unistd.h> // Include for pause() function
#include "router.hpp"

// Global router instance for signal handler
Router* g_router = nullptr;

/**
 * @brief Signal handler function
 * @param sig Signal number
 */
void signal_handler(int sig) {
    std::cout << "Received signal " << sig << ", stopping router..." << std::endl;
    if (g_router != nullptr) {
        g_router->Stop();
    }
    exit(0);
}

/**
 * @brief Main function
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Set signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);

    // Create router configuration
    RouterConfig config;

    // Override default configuration if arguments provided
    if (argc >= 3) {
        config.receiving_interface = argv[1];
        config.sending_interface = argv[2];
    }

    if (argc >= 4) {
        config.next_router = argv[3];
    }

    // Create router instance
    Router router(config);
    g_router = &router;

    // Initialize router
    std::cout << "Initializing router..." << std::endl;
    if (router.Initialize() < 0) {
        std::cerr << "Failed to initialize router" << std::endl;
        return 1;
    }

    // Run router
    std::cout << "Starting router..." << std::endl;
    if (router.Run() < 0) {
        std::cerr << "Failed to start router" << std::endl;
        return 1;
    }

    std::cout << "Router running. Press Ctrl+C to stop." << std::endl;

    // Wait for termination signal
    while (true) {
        pause();
    }

    return 0;
}