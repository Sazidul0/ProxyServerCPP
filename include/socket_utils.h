#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

#include <string>

class SocketUtils {
public:
    // Socket creation and binding
    static int create_socket();
    static bool bind_socket(int socket_fd, int port);
    static bool listen_on_socket(int socket_fd);
    
    // Connection management
    static int accept_connection(int server_socket);
    static bool connect_to_host(int socket_fd, const std::string& host, int port);
    
    // Data transfer
    static int send_data(int socket_fd, const char* data, int length);
    static int receive_data(int socket_fd, char* buffer, int buffer_size);
    
    // Cleanup
    static void close_socket(int socket_fd);
    
    // Utilities
    static std::string get_local_ip();
    static bool is_valid_socket(int socket_fd);
};

#endif // SOCKET_UTILS_H
