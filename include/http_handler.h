#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

struct HttpResponse {
    std::string version;
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpHandler {
public:
    static HttpRequest parse_request(const std::string& raw_request);
    static std::string serialize_request(const HttpRequest& request);
    
    static HttpResponse parse_response(const std::string& raw_response);
    static std::string serialize_response(const HttpResponse& response);
    
    static std::string extract_host(const HttpRequest& request);
    static int extract_port(const HttpRequest& request);
    
private:
    static std::string trim(const std::string& str);
};

#endif // HTTP_HANDLER_H
