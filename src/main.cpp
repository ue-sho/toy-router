/**
 * @file main.cpp
 * @brief Main application for the router
 */

#include "../include/router.h"
#include <iostream>
#include <csignal>
#include <fstream>
#include <cstring>
#include <unistd.h>

// Global router instance for signal handler
Router* g_router = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cerr << "Caught signal " << signal << ", shutting down..." << std::endl;
    if (g_router) {
        g_router->stop();
    }
}

// Function to disable IP forwarding in the OS
bool disableIpForwarding() {
    std::ofstream ofs("/proc/sys/net/ipv4/ip_forward");
    if (!ofs) {
        std::cerr << "Failed to open /proc/sys/net/ipv4/ip_forward: " << strerror(errno) << std::endl;
        return false;
    }

    ofs << "0";
    return ofs.good();
}

int main(int argc, char* argv[]) {
    // Default configuration
    RouterConfig config;
    config.receiving_interface = "enp0s8";
    config.sending_interface = "enp0s9";
    config.debug_output = true;
    config.next_router = "169.254.238.208";

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            config.receiving_interface = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            config.sending_interface = argv[++i];
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            config.next_router = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0) {
            config.debug_output = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -r <interface>  Receiving interface name (default: enp0s8)" << std::endl;
            std::cout << "  -s <interface>  Sending interface name (default: enp0s9)" << std::endl;
            std::cout << "  -n <ip>         Next hop router IP address (default: 169.254.238.208)" << std::endl;
            std::cout << "  -d              Enable debug output (default: on)" << std::endl;
            std::cout << "  -h, --help      Show this help message" << std::endl;
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            std::cerr << "Use -h or --help for usage information." << std::endl;
            return 1;
        }
    }

    // Check for root privileges
    if (geteuid() != 0) {
        std::cerr << "This program must be run as root." << std::endl;
        return 1;
    }

    // Disable IP forwarding in the kernel
    if (!disableIpForwarding()) {
        std::cerr << "Warning: Failed to disable IP forwarding in the kernel." << std::endl;
        std::cerr << "The router might not work as expected." << std::endl;
    }

    // Create router instance
    Router router(config);
    g_router = &router;

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize router
    if (!router.init()) {
        std::cerr << "Failed to initialize router." << std::endl;
        return 1;
    }

    // Start router
    if (!router.start()) {
        std::cerr << "Failed to start router." << std::endl;
        return 1;
    }

    std::cout << "Router started. Press Ctrl+C to stop." << std::endl;

    // Wait for signals
    while (true) {
        pause();
    }

    return 0;
}