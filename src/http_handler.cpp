#include "http_handler.h"
#include "logger.h"
#include <sstream>
#include <algorithm>

std::string HttpHandler::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

HttpRequest HttpHandler::parse_request(const std::string& raw_request) {
    HttpRequest req;
    std::istringstream iss(raw_request);
    std::string line;
    
    // Parse request line
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        line_stream >> req.method >> req.path >> req.version;
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        if (line.empty() || line == "\r") break;
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim(line.substr(0, colon));
            std::string value = trim(line.substr(colon + 1));
            req.headers[key] = value;
        }
    }
    
    // Parse body
    std::string body_content;
    while (std::getline(iss, body_content)) {
        req.body += body_content + "\n";
    }
    
    return req;
}

std::string HttpHandler::serialize_request(const HttpRequest& request) {
    std::ostringstream oss;
    oss << request.method << " " << request.path << " " << request.version << "\r\n";
    
    for (const auto& [key, value] : request.headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    if (!request.body.empty()) {
        oss << request.body;
    }
    
    return oss.str();
}

HttpResponse HttpHandler::parse_response(const std::string& raw_response) {
    HttpResponse resp;
    std::istringstream iss(raw_response);
    std::string line;
    
    // Parse status line
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        line_stream >> resp.version >> resp.status_code >> resp.status_message;
    }
    
    // Parse headers
    while (std::getline(iss, line) && line != "\r") {
        if (line.empty() || line == "\r") break;
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = trim(line.substr(0, colon));
            std::string value = trim(line.substr(colon + 1));
            resp.headers[key] = value;
        }
    }
    
    // Parse body
    std::string body_content;
    while (std::getline(iss, body_content)) {
        resp.body += body_content + "\n";
    }
    
    return resp;
}

std::string HttpHandler::serialize_response(const HttpResponse& response) {
    std::ostringstream oss;
    oss << response.version << " " << response.status_code << " "
        << response.status_message << "\r\n";
    
    for (const auto& [key, value] : response.headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    if (!response.body.empty()) {
        oss << response.body;
    }
    
    return oss.str();
}

std::string HttpHandler::extract_host(const HttpRequest& request) {
    auto it = request.headers.find("Host");
    if (it != request.headers.end()) {
        std::string host = it->second;
        size_t colon = host.find(':');
        if (colon != std::string::npos) {
            return host.substr(0, colon);
        }
        return host;
    }
    return "localhost";
}

int HttpHandler::extract_port(const HttpRequest& request) {
    auto it = request.headers.find("Host");
    if (it != request.headers.end()) {
        std::string host = it->second;
        size_t colon = host.find(':');
        if (colon != std::string::npos) {
            return std::stoi(host.substr(colon + 1));
        }
    }
    return 80; // Default HTTP port
}
