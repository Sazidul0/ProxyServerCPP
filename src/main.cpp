#include "proxy_server.h"
#include "logger.h"
#include <iostream>
#include <signal.h>
#include <atomic>

std::atomic<bool> should_exit(false);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        should_exit = true;
    }
}

int main(int argc, char* argv[]) {
    // Set log level
    Logger::set_level(INFO);
    
    // Parse command line arguments
    int port = 8080; // Default port
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            Logger::error("Invalid port number. Using default port 8080");
        }
    }
    
    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create and start proxy server
    ProxyServer proxy(port);
    
    if (!proxy.start()) {
        Logger::error("Failed to start proxy server");
        return 1;
    }
    
    Logger::info("Proxy server running on port " + std::to_string(port));
    Logger::info("Press Ctrl+C to shutdown...");
    
    // Keep the server running until interrupted
    while (!should_exit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    Logger::info("Shutting down...");
    proxy.stop();
    
    return 0;
}
