# Proxy Server in C++

A lightweight, high-performance HTTP/HTTPS proxy server built with C++ featuring request forwarding, response caching, and multi-threaded connection handling.

## Features

- **Multi-threaded**: Handles multiple concurrent client connections efficiently
- **HTTP Request Forwarding**: Parses and forwards HTTP requests to target servers
- **HTTPS Support**: Establishes secure tunnels using the CONNECT method for encrypted traffic
- **Response Caching**: Intelligent caching system for GET requests with TTL-based expiration
- **Smart Cache Management**: Respects HTTP cache headers (Cache-Control, Expires)
- **Real-time Logging**: Comprehensive logging with performance metrics and cache status
- **Socket Management**: Efficient socket handling with proper cleanup and error handling
- **Graceful Shutdown**: Supports clean shutdown via Ctrl+C

## Project Structure

```
proxy/
├── include/              # Header files
│   ├── proxy_server.h    # Main proxy server class
│   ├── socket_utils.h    # Socket operations utility
│   ├── http_handler.h    # HTTP parsing and handling
│   ├── cache_manager.h   # Response caching system
│   └── logger.h          # Logging utility
├── src/                  # Source files
│   ├── main.cpp          # Application entry point
│   ├── proxy_server.cpp  # Proxy logic with CONNECT tunneling
│   ├── socket_utils.cpp  # Socket operations
│   ├── http_handler.cpp  # HTTP parsing
│   ├── cache_manager.cpp # Caching logic
│   └── logger.cpp        # Logging
├── build/               # Build directory
├── CMakeLists.txt       # CMake configuration
└── README.md            # This file
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
# HTTP requests (cached)
curl -x http://localhost:9194 http://httpbin.org/get
wget -e use_proxy=on -e http_proxy=localhost:9194 http://httpbin.org/get

# HTTPS requests (tunneled via CONNECT)
curl -x http://localhost:9194 https://www.example.com
wget -e use_proxy=on -e http_proxy=localhost:9194 https://www.example.com
```

## Caching Behavior

### What Gets Cached
- ✅ **HTTP GET requests** with 2xx responses
- ✅ **Respects HTTP headers**: Cache-Control, Expires
- ❌ **HTTPS** (encrypted tunnel - can't cache)
- ❌ **POST/PUT/DELETE** (unsafe operations)
- ❌ Responses with no-cache/no-store directives

### Cache Performance

**First request (cache miss):**
```
[INFO] ➤ CACHE MISS - Will fetch from server
[INFO] Resolving example.com:80...
[INFO] ✓ Connected in 250ms
[INFO] ✓ Response received and transferred in 1200ms
[INFO] 💾 CACHED - Saved to cache (TTL: 300s)
```

**Second request (cache hit):**
```
[INFO] ✓ CACHE HIT - Retrieved in 0ms
```

### Example: Verify Caching Works
```bash
# Start proxy
./build/bin/proxy_server 9194

# Terminal 2: First request - ~1-2 seconds
wget -e use_proxy=on -e http_proxy=localhost:9194 http://httpbin.org/get -O file1.html

# Second request - instant (< 1ms from cache)
wget -e use_proxy=on -e http_proxy=localhost:9194 http://httpbin.org/get -O file2.html

# Watch proxy logs to see CACHE HIT!
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
- CONNECT method support for HTTPS tunneling

### CacheManager
Manages HTTP response caching:
- Thread-safe cache storage with mutex protection
- Automatic TTL-based expiration
- Smart key generation (normalizes URLs)
- Respects HTTP cache headers
- Performance metrics logging

### Logger
Provides detailed logging:
- Log levels: DEBUG, INFO, WARNING, ERROR
- Timestamp and performance metrics
- Cache hit/miss indicators
- Connection and transfer timing

## Design

The proxy server uses:
- **Multi-threading**: Each client connection is handled in a separate thread
- **Socket forwarding**: Direct socket-to-socket data forwarding for efficient proxying
- **HTTP parsing**: Comprehensive HTTP protocol support including CONNECT tunneling
- **In-memory caching**: Fast retrieval of cached responses with thread-safe operations
- **Header-aware caching**: Respects server cache directives for intelligent TTL management

## Current Limitations

- HTTPS caching: HTTPS uses encrypted tunnels (CONNECT), so responses can't be cached
- No SSL/TLS termination: Would require certificate management
- No authentication/authorization
- No connection pooling
- No compression support
- Single-machine deployment only

## Possible Future Improvements

- SSL/TLS termination with certificate spoofing (for HTTPS caching in enterprise environments)
- Connection pooling and keep-alive
- Load balancing across multiple servers
- Rate limiting and bandwidth throttling
- Request filtering and blocking rules
- Configuration file support (JSON/YAML)
- Persistent disk-based caching
- Performance optimization (zero-copy, async I/O)
- Docker containerization
- Metrics and monitoring endpoints

## Performance Tips

1. **Adjust TTL**: Modify cache TTL in `cache_manager.cpp` for your use case
2. **Monitor Cache**: Watch logs for cache hit ratio to optimize performance
3. **Use HTTP**: HTTP traffic will be cached; HTTPS is always tunneled
4. **Tune Port**: Run on different ports to prevent conflicts

## Troubleshooting

**Port already in use:**
```bash
lsof -ti:9194 | xargs kill -9  # Replace 9194 with your port
```

**Cache not working:**
- Check logs for "CACHE MISS" vs "CACHE HIT"
- Verify response has 2xx status code
- Check if server sent no-cache headers
- Ensure requests are identical (same URL/path)

**Build issues:**
```bash
cd build
make clean
cmake ..
make
```

## License

This project is provided as-is for educational and production use.


