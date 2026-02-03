#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

// Represents a location block
struct LocationConfig {
    std::string path;                      // "/" or "/upload"
    std::string root;                      // Custom root for this location
    std::string index;                     // Custom index file for this location
    std::vector<std::string> methods;      // ["GET", "POST"]
    bool autoindex;                        // true/false
    std::string upload_store;              // "./www/uploads"
    std::map<std::string, std::string> cgi_handlers;  // {".py": "/usr/bin/python3", ".php": "/usr/bin/php-cgi"}
    size_t client_max_body_size;           // Override for this location (0 = use server default)
    int redirect_code;                     // 301, 302, etc. (0 = no redirect)
    std::string redirect_url;              // URL to redirect to
    
    LocationConfig() : autoindex(false), client_max_body_size(0), redirect_code(0) {}
    
    // Helper to get CGI interpreter for a given extension
    std::string getCGIPath(const std::string& ext) const {
        std::map<std::string, std::string>::const_iterator it = cgi_handlers.find(ext);
        return (it != cgi_handlers.end()) ? it->second : "";
    }
    
    // Check if extension is a CGI type
    bool isCGIExtension(const std::string& ext) const {
        return cgi_handlers.find(ext) != cgi_handlers.end();
    }
};

// Represents a server block
struct ServerConfig {
    int port;                              // 8080
    std::string server_name;               // "localhost"
    std::string root;                      // "./www"
    std::string index;                     // "index.html"
    size_t client_max_body_size;           // 10485760 (10M in bytes)
    std::map<int, std::string> error_pages; // {404: "/404.html"}
    std::vector<LocationConfig> locations;
    
    ServerConfig() : port(8080), client_max_body_size(1048576) {} // Default 1M
};

// Main config class
class Config {
private:
    std::vector<ServerConfig> servers;
    
public:
    Config();
    bool parse(const std::string& filename);
    const std::vector<ServerConfig>& getServers() const { return servers; }
    void print() const;
    
private:
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    size_t parseSize(const std::string& size_str);
    bool isNumber(const std::string& str);
    bool validatePorts() const;
};

#endif