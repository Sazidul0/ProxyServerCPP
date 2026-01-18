#include "proxy_server.h"
#include "socket_utils.h"
#include "http_handler.h"
#include "logger.h"
#include <thread>
#include <chrono>
#include <cstring>

ProxyServer::ProxyServer(int port) : server_socket(-1), port(port), running(false) {}

ProxyServer::~ProxyServer() {
    stop();
}

bool ProxyServer::start() {
    server_socket = SocketUtils::create_socket();
    if (server_socket < 0) {
        return false;
    }
    
    if (!SocketUtils::bind_socket(server_socket, port)) {
        SocketUtils::close_socket(server_socket);
        return false;
    }
    
    if (!SocketUtils::listen_on_socket(server_socket)) {
        SocketUtils::close_socket(server_socket);
        return false;
    }
    
    running = true;
    server_thread = std::thread(&ProxyServer::start_listening, this);
    Logger::info("Proxy server started on port " + std::to_string(port));
    
    return true;
}

void ProxyServer::stop() {
    running = false;
    if (server_socket >= 0) {
        SocketUtils::close_socket(server_socket);
    }
    if (server_thread.joinable()) {
        server_thread.join();
    }
    Logger::info("Proxy server stopped");
}

void ProxyServer::start_listening() {
    while (running) {
        int client_socket = SocketUtils::accept_connection(server_socket);
        if (client_socket < 0) {
            if (running) {
                continue;
            } else {
                break;
            }
        }
        
        // Handle each client in a separate thread
        std::thread(&ProxyServer::handle_client, this, client_socket).detach();
    }
}

void ProxyServer::handle_client(int client_socket) {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    
    // Receive request from client
    int received = SocketUtils::receive_data(client_socket, buffer, BUFFER_SIZE - 1);
    if (received <= 0) {
        SocketUtils::close_socket(client_socket);
        return;
    }
    
    buffer[received] = '\0';
    std::string request_data(buffer);
    
    Logger::debug("Received request from client");
    
    // Parse HTTP request
    HttpRequest request = HttpHandler::parse_request(request_data);
    
    // Extract target host and port
    std::string target_host = HttpHandler::extract_host(request);
    int target_port = HttpHandler::extract_port(request);
    
    Logger::info("Forwarding request to " + target_host + ":" + std::to_string(target_port));
    
    // Connect to target server
    int target_socket = SocketUtils::create_socket();
    if (target_socket < 0 || !SocketUtils::connect_to_host(target_socket, target_host, target_port)) {
        Logger::error("Failed to connect to target server");
        SocketUtils::close_socket(client_socket);
        SocketUtils::close_socket(target_socket);
        return;
    }
    
    // Forward request to target server
    std::string serialized_request = HttpHandler::serialize_request(request);
    SocketUtils::send_data(target_socket, serialized_request.c_str(), serialized_request.length());
    
    // Forward response back to client
    while (true) {
        int response_received = SocketUtils::receive_data(target_socket, buffer, BUFFER_SIZE);
        if (response_received <= 0) {
            break;
        }
        
        SocketUtils::send_data(client_socket, buffer, response_received);
    }
    
    Logger::info("Request forwarding completed");
    
    // Clean up
    SocketUtils::close_socket(client_socket);
    SocketUtils::close_socket(target_socket);
}

int ProxyServer::get_port() const {
    return port;
}
