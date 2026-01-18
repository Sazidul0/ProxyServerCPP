#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include "http_handler.h"

struct CachedResponse {
    HttpResponse response;
    std::chrono::system_clock::time_point cached_time;
    int ttl_seconds; // Time to live in seconds
    
    bool is_expired() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - cached_time);
        return duration.count() > ttl_seconds;
    }
};

class CacheManager {
private:
    std::map<std::string, CachedResponse> cache;
    mutable std::mutex cache_mutex;
    bool cache_enabled;
    
    static std::string generate_cache_key(const HttpRequest& request);
    static int extract_ttl_from_headers(const std::map<std::string, std::string>& headers);

public:
    CacheManager();
    ~CacheManager();
    
    // Check if response is in cache and not expired
    bool get(const HttpRequest& request, HttpResponse& response);
    
    // Store response in cache
    void put(const HttpRequest& request, const HttpResponse& response);
    
    // Clear all cache
    void clear();
    
    // Get cache size
    size_t size() const;
    
    // Enable/disable caching
    void set_enabled(bool enabled);
    bool is_enabled() const;
};

#endif // CACHE_MANAGER_H
