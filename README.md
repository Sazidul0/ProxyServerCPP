# Proxy Server in C++

A lightweight HTTP proxy server built with C++ that forwards HTTP requests to target servers and returns responses to clients.

## Features

- **Multi-threaded**: Handles multiple client connections simultaneously
- **HTTP Request Forwarding**: Parses and forwards HTTP requests to target servers
- **Logging**: Comprehensive logging system with multiple log levels
- **Socket Management**: Efficient socket handling with proper cleanup
- **Graceful Shutdown**: Supports clean shutdown via Ctrl+C

## Project Structure

```
proxy/
├── include/              # Header files
│   ├── proxy_server.h   # Main proxy server class
│   ├── socket_utils.h   # Socket operations utility
│   ├── http_handler.h   # HTTP parsing and handling
│   └── logger.h         # Logging utility
├── src/                 # Source files
│   ├── main.cpp        # Application entry point
│   ├── proxy_server.cpp
│   ├── socket_utils.cpp
│   ├── http_handler.cpp
│   └── logger.cpp
├── build/              # Build directory
├── CMakeLists.txt      # CMake configuration
└── README.md           # This file
```

## Building

### Prerequisites

- C++17 compatible compiler (g++, clang, or MSVC)
- CMake 3.10 or higher
- POSIX-compliant system (Linux, macOS) or Windows with appropriate socket support

### Build Instructions

```bash
cd /path/to/proxy
mkdir -p build
cd build
cmake ..
make
```

### Running

```bash
# Run on default port 8080
./bin/proxy_server

# Run on custom port
./bin/proxy_server 3128
```

## Usage

Once the proxy server is running, configure your client to use it:

```bash
# Example using curl
curl -x http://localhost:8080 http://example.com

# Or with wget
wget -e use_proxy=on -e http_proxy=localhost:8080 http://example.com
```

## Components

### ProxyServer
Main server class that listens for incoming connections and spawns threads to handle each client.

### SocketUtils
Utility class providing socket operations including:
- Socket creation and binding
- Connection management
- Data transmission and reception
- Socket cleanup

### HttpHandler
Handles HTTP protocol operations:
- Request parsing and serialization
- Response parsing and serialization
- Header extraction (Host, Port, etc.)

### Logger
Provides configurable logging with levels: DEBUG, INFO, WARNING, ERROR

## Design

The proxy server uses:
- **Multi-threading**: Each client connection is handled in a separate thread
- **Socket forwarding**: Direct socket-to-socket data forwarding for efficient proxying
- **HTTP parsing**: Basic HTTP protocol support for request/response handling

## Limitations

- Only supports HTTP (not HTTPS)
- Basic HTTP parsing (no advanced features)
- No connection pooling or caching
- No authentication support

## Future Improvements

- HTTPS/SSL support
- Connection pooling
- Caching mechanisms
- Load balancing
- Rate limiting
- Authentication and authorization
- Configuration file support
- Performance optimization


