#include "socket_utils.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <ifaddrs.h>

int SocketUtils::create_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        Logger::error("Failed to create socket");
        return -1;
    }
    
    // Set socket options to allow reuse
    int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::warning("Failed to set socket options");
    }
    
    return socket_fd;
}

bool SocketUtils::bind_socket(int socket_fd, int port) {
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::error("Failed to bind socket on port " + std::to_string(port));
        return false;
    }
    
    Logger::info("Socket bound to port " + std::to_string(port));
    return true;
}

bool SocketUtils::listen_on_socket(int socket_fd) {
    if (listen(socket_fd, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on socket");
        return false;
    }
    Logger::info("Server listening...");
    return true;
}

int SocketUtils::accept_connection(int server_socket) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (client_socket < 0) {
        Logger::error("Failed to accept connection");
        return -1;
    }
    
    Logger::info("New connection from " + std::string(inet_ntoa(client_addr.sin_addr)));
    return client_socket;
}

bool SocketUtils::connect_to_host(int socket_fd, const std::string& host, int port) {
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        Logger::error("Failed to resolve host: " + host);
        return false;
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        Logger::error("Failed to connect to " + host + ":" + std::to_string(port));
        return false;
    }
    
    Logger::info("Connected to " + host + ":" + std::to_string(port));
    return true;
}

int SocketUtils::send_data(int socket_fd, const char* data, int length) {
    int sent = send(socket_fd, data, length, 0);
    if (sent < 0) {
        Logger::error("Failed to send data");
        return -1;
    }
    return sent;
}

int SocketUtils::receive_data(int socket_fd, char* buffer, int buffer_size) {
    int received = recv(socket_fd, buffer, buffer_size, 0);
    if (received < 0) {
        Logger::error("Failed to receive data");
        return -1;
    }
    return received;
}

void SocketUtils::close_socket(int socket_fd) {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

std::string SocketUtils::get_local_ip() {
    struct ifaddrs* ifaddr;
    std::string ip = "127.0.0.1";
    
    if (getifaddrs(&ifaddr) == -1) {
        return ip;
    }
    
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            if (std::string(ifa->ifa_name) != "lo") {
                ip = inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr);
                break;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return ip;
}

bool SocketUtils::is_valid_socket(int socket_fd) {
    return socket_fd >= 0;
}
