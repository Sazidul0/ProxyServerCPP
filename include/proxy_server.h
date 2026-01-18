#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>

class ProxyServer {
private:
    int server_socket;
    int port;
    std::atomic<bool> running;
    std::thread server_thread;

    void start_listening();
    void handle_client(int client_socket);
    static void forward_data(int source, int dest);

public:
    ProxyServer(int port);
    ~ProxyServer();
    
    bool start();
    void stop();
    int get_port() const;
};

#endif // PROXY_SERVER_H
