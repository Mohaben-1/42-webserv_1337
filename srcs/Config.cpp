#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstdlib>

Config::Config() {}

std::string Config::trim(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(str[start]))
        start++;
    while (end > start && std::isspace(str[end - 1]))
        end--;
    
    return str.substr(start, end - start);
}

std::vector<std::string> Config::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    
    while (std::getline(tokenStream, token, delimiter)) {
        token = trim(token);
        if (!token.empty())
            tokens.push_back(token);
    }
    
    return tokens;
}

size_t Config::parseSize(const std::string& size_str) {
    if (size_str.empty())
        return 0;
    
    size_t multiplier = 1;
    std::string num_str = size_str;
    
    char last_char = size_str[size_str.length() - 1];
    if (last_char == 'K' || last_char == 'k') {
        multiplier = 1024;
        num_str = size_str.substr(0, size_str.length() - 1);
    } else if (last_char == 'M' || last_char == 'm') {
        multiplier = 1024 * 1024;
        num_str = size_str.substr(0, size_str.length() - 1);
    } else if (last_char == 'G' || last_char == 'g') {
        multiplier = 1024 * 1024 * 1024;
        num_str = size_str.substr(0, size_str.length() - 1);
    }
    
    return std::atoi(num_str.c_str()) * multiplier;
}

bool Config::isNumber(const std::string& str) {
    for (size_t i = 0; i < str.length(); i++) {
        if (!std::isdigit(str[i]))
            return false;
    }
    return !str.empty();
}

bool Config::parse(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    ServerConfig* current_server = NULL;
    LocationConfig* current_location = NULL;
    int brace_level = 0;
    bool in_server = false;
    bool in_location = false;
    
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        line = trim(line);
        if (line.empty())
            continue;
        
        // Remove semicolon at the end
        if (line[line.length() - 1] == ';') {
            line = line.substr(0, line.length() - 1);
            line = trim(line);
        }
        
        // Handle braces
        if (line.find('{') != std::string::npos) {
            brace_level++;
            
            if (line.find("server") == 0) {
                servers.push_back(ServerConfig());
                current_server = &servers.back();
                in_server = true;
            }
            else if (line.find("location") == 0) {
                std::vector<std::string> tokens = split(line, ' ');
                if (tokens.size() >= 2 && current_server) {
                    current_server->locations.push_back(LocationConfig());
                    current_location = &current_server->locations.back();
                    current_location->path = tokens[1];
                    in_location = true;
                }
            }
            continue;
        }
        
        if (line.find('}') != std::string::npos) {
            brace_level--;
            if (in_location) {
                in_location = false;
                current_location = NULL;
            } else if (in_server) {
                in_server = false;
                current_server = NULL;
            }
            continue;
        }
        
        // Parse directives
        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.empty())
            continue;
        
        std::string directive = tokens[0];
        
        // Server-level directives
        if (in_server && !in_location && current_server) {
            if (directive == "listen" && tokens.size() >= 2) {
                current_server->port = std::atoi(tokens[1].c_str());
            }
            else if (directive == "server_name" && tokens.size() >= 2) {
                current_server->server_name = tokens[1];
            }
            else if (directive == "root" && tokens.size() >= 2) {
                current_server->root = tokens[1];
            }
            else if (directive == "index" && tokens.size() >= 2) {
                current_server->index = tokens[1];
            }
            else if (directive == "client_max_body_size" && tokens.size() >= 2) {
                current_server->client_max_body_size = parseSize(tokens[1]);
            }
            else if (directive == "error_page" && tokens.size() >= 3) {
                // error_page 404 /404.html
                // or error_page 500 502 503 /50x.html
                std::string path = tokens[tokens.size() - 1]; // Last token is the path
                for (size_t i = 1; i < tokens.size() - 1; i++) {
                    if (isNumber(tokens[i])) {
                        int code = std::atoi(tokens[i].c_str());
                        current_server->error_pages[code] = path;
                    }
                }
            }
        }
        
        // Location-level directives
        if (in_location && current_location) {
            if (directive == "methods" && tokens.size() >= 2) {
                for (size_t i = 1; i < tokens.size(); i++) {
                    current_location->methods.push_back(tokens[i]);
                }
            }
            else if (directive == "root" && tokens.size() >= 2) {
                current_location->root = tokens[1];
            }
            else if (directive == "index" && tokens.size() >= 2) {
                current_location->index = tokens[1];
            }
            else if (directive == "autoindex" && tokens.size() >= 2) {
                current_location->autoindex = (tokens[1] == "on");
            }
            else if (directive == "upload_store" && tokens.size() >= 2) {
                current_location->upload_store = tokens[1];
            }
            else if (directive == "cgi" && tokens.size() >= 3) {
                // cgi .py /usr/bin/python3
                // cgi .php /usr/bin/php-cgi
                current_location->cgi_handlers[tokens[1]] = tokens[2];
            }
            else if (directive == "cgi_extension" && tokens.size() >= 2) {
                // Legacy support: cgi_extension .py with cgi_path
                // Store temporarily, will be paired with cgi_path
                current_location->cgi_handlers[tokens[1]] = "";
            }
            else if (directive == "cgi_path" && tokens.size() >= 2) {
                // Legacy support: set path for last empty-path extension
                for (std::map<std::string, std::string>::iterator it = current_location->cgi_handlers.begin();
                     it != current_location->cgi_handlers.end(); ++it) {
                    if (it->second.empty()) {
                        it->second = tokens[1];
                    }
                }
            }
            else if (directive == "client_max_body_size" && tokens.size() >= 2) {
                current_location->client_max_body_size = parseSize(tokens[1]);
            }
            else if (directive == "return" && tokens.size() >= 3) {
                // return 301 http://example.com/new-url
                current_location->redirect_code = std::atoi(tokens[1].c_str());
                current_location->redirect_url = tokens[2];
            }
        }
    }
    
    file.close();
    
    if (servers.empty()) {
        std::cerr << "Error: No server blocks found in config" << std::endl;
        return false;
    }
    
    // Validate that there are no duplicate port+server_name combinations
    if (!validatePorts()) {
        return false;
    }
    
    return true;
}

bool Config::validatePorts() const {
    // Check for duplicate port + server_name combinations
    // Same port is allowed only if server_names are different (virtual hosting)
    for (size_t i = 0; i < servers.size(); i++) {
        for (size_t j = i + 1; j < servers.size(); j++) {
            if (servers[i].port == servers[j].port &&
                servers[i].server_name == servers[j].server_name) {
                std::cerr << "Error: Duplicate server configuration detected" << std::endl;
                std::cerr << "  Port " << servers[i].port << " with server_name '" 
                          << servers[i].server_name << "' is defined multiple times" << std::endl;
                return false;
            }
        }
    }
    return true;
}

void Config::print() const {
    std::cout << "\n";
    std::cout << "    ╦ ╦┌─┐┌┐ ┌─┐┌─┐┬─┐┬  ┬" << std::endl;
    std::cout << "    ║║║├┤ ├┴┐└─┐├┤ ├┬┘└┐┌┘" << std::endl;
    std::cout << "    ╚╩╝└─┘└─┘└─┘└─┘┴└─ └┘ " << std::endl;
    std::cout << "         42 HTTP Server\n" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "  Servers: " << servers.size() << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    for (size_t i = 0; i < servers.size(); i++) {
        const ServerConfig& srv = servers[i];
        
        std::cout << "┌─ Server #" << (i + 1) << " ─────────────────────────────" << std::endl;
        std::cout << "│  Listen:    " << srv.server_name << ":" << srv.port << std::endl;
        std::cout << "│  Root:      " << srv.root << std::endl;
        std::cout << "│  Index:     " << srv.index << std::endl;
        
        std::cout << "│  Max Body:  ";
        if (srv.client_max_body_size >= 1048576)
            std::cout << (srv.client_max_body_size / 1048576) << "MB" << std::endl;
        else if (srv.client_max_body_size >= 1024)
            std::cout << (srv.client_max_body_size / 1024) << "KB" << std::endl;
        else
            std::cout << srv.client_max_body_size << "B" << std::endl;
        
        std::cout << "└──────────────────────────────────────\n" << std::endl;
    }
}