#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>

class Response {
private:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::vector<std::string> cookies;  // Set-Cookie headers
    std::string body;

public:
    Response();
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
    std::string toString() const;
    
    // Cookie support
    void setCookie(const std::string& name, const std::string& value, 
                   int max_age = -1, const std::string& path = "/",
                   bool http_only = true, bool secure = false);
    void deleteCookie(const std::string& name, const std::string& path = "/");
    
    // Helper function to get Content-Type from file extension
    static std::string getContentType(const std::string& path);
};

#endif