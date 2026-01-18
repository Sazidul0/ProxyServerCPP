#include "cache_manager.h"
#include "logger.h"
#include <sstream>
#include <algorithm>

CacheManager::CacheManager() : cache_enabled(true) {}

CacheManager::~CacheManager() {
    clear();
}

std::string CacheManager::generate_cache_key(const HttpRequest& request) {
    // Create a unique key based on method, host, and path
    std::ostringstream oss;
    
    auto it = request.headers.find("Host");
    std::string host = (it != request.headers.end()) ? it->second : "unknown";
    
    // Normalize path - remove scheme and host if present (proxy-style request)
    std::string path = request.path;
    size_t scheme_end = path.find("://");
    if (scheme_end != std::string::npos) {
        // This is an absolute URL like "http://httpbin.org/get"
        size_t path_start = path.find('/', scheme_end + 3);
        if (path_start != std::string::npos) {
            path = path.substr(path_start);
        } else {
            path = "/";
        }
    }
    
    oss << request.method << ":" << host << ":" << path;
    return oss.str();
}

int CacheManager::extract_ttl_from_headers(const std::map<std::string, std::string>& headers) {
    // Look for Cache-Control header
    auto it = headers.find("Cache-Control");
    if (it != headers.end()) {
        std::string cache_control = it->second;
        
        // Look for max-age directive
        size_t max_age_pos = cache_control.find("max-age=");
        if (max_age_pos != std::string::npos) {
            std::string age_str = cache_control.substr(max_age_pos + 8);
            size_t comma_pos = age_str.find(',');
            if (comma_pos != std::string::npos) {
                age_str = age_str.substr(0, comma_pos);
            }
            try {
                return std::stoi(age_str);
            } catch (...) {
                // Return default if parsing fails
            }
        }
        
        // Check if no-cache or no-store is present
        if (cache_control.find("no-cache") != std::string::npos ||
            cache_control.find("no-store") != std::string::npos ||
            cache_control.find("private") != std::string::npos) {
            return 0; // Don't cache
        }
    }
    
    // Check Expires header
    it = headers.find("Expires");
    if (it != headers.end()) {
        // For simplicity, use a default TTL for pages with Expires header
        return 3600; // 1 hour default
    }
    
    // Default cache time: 5 minutes for cacheable content
    // Only cache GET requests and successful responses (2xx)
    return 300;
}

bool CacheManager::get(const HttpRequest& request, HttpResponse& response) {
    if (!cache_enabled || request.method != "GET") {
        return false;
    }
    
    std::string key = generate_cache_key(request);
    
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        if (it->second.is_expired()) {
            Logger::info("⏱ Cache entry EXPIRED for: " + key);
            cache.erase(it);
            return false;
        }
        
        response = it->second.response;
        Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Logger::info("✓ CACHE HIT - Retrieved in 0ms");
        Logger::info("Key: " + key);
        Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        return true;
    }
    
    Logger::info("➤ CACHE MISS - Will fetch from server");
    Logger::info("  Key: " + key);
    return false;
}

void CacheManager::put(const HttpRequest& request, const HttpResponse& response) {
    if (!cache_enabled || request.method != "GET") {
        return;
    }
    
    // Only cache successful responses (2xx status codes)
    if (response.status_code < 200 || response.status_code >= 300) {
        return;
    }
    
    int ttl = extract_ttl_from_headers(response.headers);
    if (ttl <= 0) {
        Logger::debug("Response not cacheable (no-cache/no-store headers)");
        return;
    }
    
    std::string key = generate_cache_key(request);
    
    std::lock_guard<std::mutex> lock(cache_mutex);
    
    CachedResponse cached;
    cached.response = response;
    cached.cached_time = std::chrono::system_clock::now();
    cached.ttl_seconds = ttl;
    
    cache[key] = cached;
    
    Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Logger::info("💾 CACHED - Saved to cache");
    Logger::info("Key: " + key);
    Logger::info("TTL: " + std::to_string(ttl) + "s | Size: " + std::to_string(response.body.length()) + " bytes");
    Logger::info("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

void CacheManager::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache.clear();
    Logger::info("Cache cleared");
}

size_t CacheManager::size() const {
    std::lock_guard<std::mutex> lock(cache_mutex);
    return cache.size();
}

void CacheManager::set_enabled(bool enabled) {
    cache_enabled = enabled;
    Logger::info(std::string("Caching ") + (enabled ? "enabled" : "disabled"));
}

bool CacheManager::is_enabled() const {
    return cache_enabled;
}
