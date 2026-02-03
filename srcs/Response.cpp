#include "Response.hpp"

Response::Response() : status_code(200), status_message("OK") {}

void Response::setStatus(int code, const std::string& message) {
    status_code = code;
    status_message = message;
}

void Response::setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void Response::setBody(const std::string& content) {
    body = content;
    
    // Automatically set Content-Length
    std::ostringstream oss;
    oss << content.length();
    setHeader("Content-Length", oss.str());
}

std::string Response::toString() const {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code << " " << status_message << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }
    
    // Set-Cookie headers (can have multiple)
    for (size_t i = 0; i < cookies.size(); i++) {
        response << "Set-Cookie: " << cookies[i] << "\r\n";
    }
    
    // Empty line separates headers from body
    response << "\r\n";
    
    // Body
    response << body;
    
    return response.str();
}

// Set a cookie with optional attributes
void Response::setCookie(const std::string& name, const std::string& value,
                         int max_age, const std::string& path,
                         bool http_only, bool secure) {
    std::ostringstream cookie;
    cookie << name << "=" << value;
    
    if (!path.empty()) {
        cookie << "; Path=" << path;
    }
    
    if (max_age >= 0) {
        cookie << "; Max-Age=" << max_age;
    }
    
    if (http_only) {
        cookie << "; HttpOnly";
    }
    
    if (secure) {
        cookie << "; Secure";
    }
    
    cookie << "; SameSite=Lax";
    
    cookies.push_back(cookie.str());
}

// Delete a cookie by setting it expired
void Response::deleteCookie(const std::string& name, const std::string& path) {
    std::ostringstream cookie;
    cookie << name << "=deleted; Path=" << path << "; Max-Age=0";
    cookies.push_back(cookie.str());
}

// Get Content-Type based on file extension
std::string Response::getContentType(const std::string& path) {
    // Find the extension
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream"; // Default for unknown types
    }
    
    std::string ext = path.substr(dot_pos);
    
    // Map extensions to MIME types
    if (ext == ".html" || ext == ".htm")
        return "text/html";
    else if (ext == ".css")
        return "text/css";
    else if (ext == ".js")
        return "text/javascript";
    else if (ext == ".json")
        return "application/json";
    else if (ext == ".txt")
        return "text/plain";
    else if (ext == ".jpg" || ext == ".jpeg")
        return "image/jpeg";
    else if (ext == ".png")
        return "image/png";
    else if (ext == ".gif")
        return "image/gif";
    else if (ext == ".pdf")
        return "application/pdf";
    else if (ext == ".xml")
        return "application/xml";
    else
        return "application/octet-stream";
}