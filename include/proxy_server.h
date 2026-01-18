#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "http_handler.h"
#include "cache_manager.h"

class ProxyServer {
private:
    int server_socket;
    int port;
    std::atomic<bool> running;
    std::thread server_thread;
    std::shared_ptr<CacheManager> cache_manager;

    void start_listening();
    void handle_client(int client_socket);
    void handle_connect_tunnel(int client_socket, const HttpRequest& request);
    static void forward_data(int source, int dest);

public:
    ProxyServer(int port);
    ~ProxyServer();
    
    bool start();
    void stop();
    int get_port() const;
    std::shared_ptr<CacheManager> get_cache_manager() const { return cache_manager; }
};

#endif // PROXY_SERVER_H
