#include "proxy_server.h"
#include "socket_utils.h"
#include "http_handler.h"
#include "logger.h"
#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>

ProxyServer::ProxyServer(int port) : server_socket(-1), port(port), running(false) {
    cache_manager = std::make_shared<CacheManager>();
}

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
    
    // Check if this is a CONNECT request (for HTTPS tunneling)
    if (request.method == "CONNECT") {
        handle_connect_tunnel(client_socket, request);
        return;
    }
    
    // Check cache for GET requests
    HttpResponse cached_response;
    if (request.method == "GET" && cache_manager->get(request, cached_response)) {
        // Serve from cache
        auto cache_start = std::chrono::high_resolution_clock::now();
        std::string serialized_response = HttpHandler::serialize_response(cached_response);
        SocketUtils::send_data(client_socket, serialized_response.c_str(), serialized_response.length());
        auto cache_end = std::chrono::high_resolution_clock::now();
        auto cache_duration = std::chrono::duration_cast<std::chrono::milliseconds>(cache_end - cache_start);
        
        Logger::info("✓ Retrieved from CACHE in " + std::to_string(cache_duration.count()) + "ms");
        SocketUtils::close_socket(client_socket);
        return;
    }
    
    // Extract target host and port
    std::string target_host = HttpHandler::extract_host(request);
    int target_port = HttpHandler::extract_port(request);
    
    Logger::info("Resolving " + target_host + ":" + std::to_string(target_port) + "...");
    
    // Connect to target server and measure time
    auto resolve_start = std::chrono::high_resolution_clock::now();
    int target_socket = SocketUtils::create_socket();
    if (target_socket < 0 || !SocketUtils::connect_to_host(target_socket, target_host, target_port)) {
        Logger::error("Failed to connect to target server");
        SocketUtils::close_socket(client_socket);
        SocketUtils::close_socket(target_socket);
        return;
    }
    auto resolve_end = std::chrono::high_resolution_clock::now();
    auto resolve_duration = std::chrono::duration_cast<std::chrono::milliseconds>(resolve_end - resolve_start);
    
    Logger::info("✓ Connected in " + std::to_string(resolve_duration.count()) + "ms");
    
    // Forward request to target server
    std::string serialized_request = HttpHandler::serialize_request(request);
    SocketUtils::send_data(target_socket, serialized_request.c_str(), serialized_request.length());
    
    // Collect response headers first
    auto transfer_start = std::chrono::high_resolution_clock::now();
    std::string full_response;
    std::string headers_only;
    bool headers_complete = false;
    
    while (true) {
        int response_received = SocketUtils::receive_data(target_socket, buffer, BUFFER_SIZE);
        if (response_received <= 0) {
            break;
        }
        
        full_response.append(buffer, response_received);
        SocketUtils::send_data(client_socket, buffer, response_received);
        
        // Cache as soon as we have complete headers
        if (!headers_complete && full_response.find("\r\n\r\n") != std::string::npos) {
            headers_complete = true;
            
            // Parse and cache the response if it's a GET request
            if (request.method == "GET") {
                HttpResponse response = HttpHandler::parse_response(full_response);
                cache_manager->put(request, response);
                Logger::info("💾 Response headers received - CACHED immediately");
            }
        }
    }
    
    if (!headers_complete && request.method == "GET" && !full_response.empty()) {
        // Fallback: cache even if headers weren't complete
        HttpResponse response = HttpHandler::parse_response(full_response);
        cache_manager->put(request, response);
    }
    
    auto transfer_end = std::chrono::high_resolution_clock::now();
    auto transfer_duration = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_end - transfer_start);
    
    Logger::info("✓ Response received and transferred in " + std::to_string(transfer_duration.count()) + "ms");
    Logger::info("Request completed (Response size: " + std::to_string(full_response.length()) + " bytes)");
    
    // Clean up
    SocketUtils::close_socket(client_socket);
    SocketUtils::close_socket(target_socket);
}

void ProxyServer::handle_connect_tunnel(int client_socket, const HttpRequest& request) {
    // CONNECT method is used for HTTPS tunneling
    // Format: CONNECT host:port HTTP/1.1
    
    // Parse the host:port from the path
    std::string host_port = request.path;
    size_t colon = host_port.find(':');
    
    if (colon == std::string::npos) {
        Logger::error("Invalid CONNECT request format");
        SocketUtils::close_socket(client_socket);
        return;
    }
    
    std::string target_host = host_port.substr(0, colon);
    int target_port = std::stoi(host_port.substr(colon + 1));
    
    Logger::info("CONNECT tunnel requested to " + target_host + ":" + std::to_string(target_port));
    
    // Connect to target server
    int target_socket = SocketUtils::create_socket();
    if (target_socket < 0 || !SocketUtils::connect_to_host(target_socket, target_host, target_port)) {
        Logger::error("Failed to connect to target server for CONNECT tunnel");
        const char* error_response = "HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n";
        SocketUtils::send_data(client_socket, error_response, strlen(error_response));
        SocketUtils::close_socket(client_socket);
        SocketUtils::close_socket(target_socket);
        return;
    }
    
    // Send 200 OK response to client
    const char* success_response = "HTTP/1.1 200 Connection Established\r\nConnection: close\r\n\r\n";
    SocketUtils::send_data(client_socket, success_response, strlen(success_response));
    
    Logger::info("CONNECT tunnel established");
    
    // Bidirectional tunnel: forward data between client and target
    std::thread client_to_target([client_socket, target_socket]() {
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        while (true) {
            int received = SocketUtils::receive_data(client_socket, buffer, BUFFER_SIZE);
            if (received <= 0) break;
            SocketUtils::send_data(target_socket, buffer, received);
        }
    });
    
    std::thread target_to_client([client_socket, target_socket]() {
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        while (true) {
            int received = SocketUtils::receive_data(target_socket, buffer, BUFFER_SIZE);
            if (received <= 0) break;
            SocketUtils::send_data(client_socket, buffer, received);
        }
    });
    
    client_to_target.join();
    target_to_client.join();
    
    Logger::info("CONNECT tunnel closed");
    
    // Clean up
    SocketUtils::close_socket(client_socket);
    SocketUtils::close_socket(target_socket);
}

int ProxyServer::get_port() const {
    return port;
}
