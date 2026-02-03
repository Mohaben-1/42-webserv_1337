#include "ServerManager.hpp"
#include "Response.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

ServerManager::ServerManager() {}

ServerManager::~ServerManager() {
    stop();
}

bool ServerManager::initServers(const std::vector<ServerConfig>& configs) {
    // Track which ports have been bound (for virtual hosting support)
    std::map<int, int> port_to_server_index;  // port -> first server index using it
    
    // Create and start each server
    for (size_t i = 0; i < configs.size(); i++) {
        int port = configs[i].port;
        
        // Check if this port is already bound by another server
        if (port_to_server_index.find(port) != port_to_server_index.end()) {
            // Port already in use - this is virtual hosting
            Server* server = new Server(configs[i]);
            servers.push_back(server);
            
            // Map to the same fd as the first server on this port
            int first_server_idx = port_to_server_index[port];
            int shared_fd = servers[first_server_idx]->getServerFd();
            fd_to_server[shared_fd] = first_server_idx;
            continue;
        }
        
        // New port - create and bind
        Server* server = new Server(configs[i]);
        
        if (!server->start()) {
            std::cerr << "Failed to start server on port " << port << std::endl;
            delete server;
            return false;
        }
        
        servers.push_back(server);
        port_to_server_index[port] = i;
        
        // Add server socket to poll
        int server_fd = server->getServerFd();
        addPollFd(server_fd, POLLIN);
        fd_to_server[server_fd] = i;
        server_fds.insert(server_fd);
    }
    
    std::cout << "Webserv ready - listening on " << servers.size() << " server(s)" << std::endl;
    
    return true;
}

void ServerManager::run() {
    time_t last_timeout_check = time(NULL);
    
    while (true) {
        // Wait for activity on any socket (with 1 second timeout for checking idle connections)
        int activity = poll(&poll_fds[0], poll_fds.size(), 1000);
        
        if (activity < 0) {
            std::cerr << "poll() error" << std::endl;
            break;
        }
        
        // Periodically check for timed-out connections
        if (time(NULL) - last_timeout_check >= 5) {
            checkTimeouts();
            last_timeout_check = time(NULL);
        }
        
        // Check each file descriptor for BOTH read and write events
        for (size_t i = 0; i < poll_fds.size(); i++) {
            int fd = poll_fds[i].fd;
            short revents = poll_fds[i].revents;
            
            // Handle errors and hangups
            if (revents & (POLLERR | POLLHUP | POLLNVAL)) {
                if (server_fds.find(fd) == server_fds.end()) {
                    // Client socket error
                    closeClient(fd);
                    continue;
                }
            }
            
            // Check for read events (POLLIN)
            if (revents & POLLIN) {
                // Check if this is a server socket (new connection)
                if (server_fds.find(fd) != server_fds.end()) {
                    // This is a server socket - new connection
                    handleNewConnection(fd_to_server[fd]);
                } else {
                    // This is a client socket - handle request
                    handleClientRequest(fd);
                }
            }
            
            // Check for write events (POLLOUT)
            if (revents & POLLOUT) {
                // Only client sockets should have POLLOUT
                if (server_fds.find(fd) == server_fds.end()) {
                    handleClientWrite(fd);
                }
            }
        }
    }
}

void ServerManager::handleNewConnection(int server_index) {
    Server* server = servers[server_index];
    int server_fd = server->getServerFd();
    
    // Accept new connection
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        std::cerr << "Accept failed on server " << (server_index + 1) << std::endl;
        return;
    }
    
    // Set client socket to non-blocking mode (only F_SETFL and O_NONBLOCK allowed on macOS)
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0) {
        close(client_fd);
        return;
    }
    
    // Add client to poll - register for BOTH read and write events
    addPollFd(client_fd, POLLIN | POLLOUT);
    fd_to_server[client_fd] = server_index;
    
    // Initialize client state for incremental parsing
    ClientState state;
    state.server_index = server_index;
    client_states[client_fd] = state;
}

void ServerManager::handleClientRequest(int client_fd) {
    // Get client state first
    std::map<int, ClientState>::iterator it = client_states.find(client_fd);
    if (it == client_states.end()) {
        std::cerr << "No state found for client fd " << client_fd << std::endl;
        closeClient(client_fd);
        return;
    }
    
    // If response is already ready, don't read more (wait for write to complete)
    if (it->second.response_ready) {
        return;
    }
    
    // Read available data from socket
    char buffer[8192];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    // Check return value properly (both -1 and 0)
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // Client closed connection
            closeClient(client_fd);
        } else {
            // bytes_read < 0: error occurred
            // For non-blocking sockets, would-block is not a real error
            // but we don't check errno - just close on any error
            closeClient(client_fd);
        }
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    ClientState& state = it->second;
    state.last_activity = time(NULL);  // Update activity timestamp
    Request& req = state.request;
    
    // Append received data to request
    req.appendData(std::string(buffer, bytes_read));
    
    // Try to parse headers if not done yet
    if (!req.isHeadersComplete()) {
        if (!req.parseHeaders()) {
            // Headers not complete yet, wait for more data
            return;
        }
        
        // Check body size limit early
        Server* server = servers[state.server_index];
        size_t max_size = server->getConfig().client_max_body_size;
        if (req.getContentLength() > max_size) {
            // Queue 413 response
            Response res;
            res.setStatus(413, "Payload Too Large");
            res.setHeader("Content-Type", "text/html");
            res.setHeader("Connection", "close");
            res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
            
            queueResponse(client_fd, res.toString());
            return;
        }
    }
    
    // Check if request is complete (headers + full body)
    if (!req.isComplete()) {
        // Still waiting for body data
        return;
    }
    
    // Request is complete, process it
    
    // Get the original server (based on which port received the connection)
    Server* original_server = servers[state.server_index];
    int port = original_server->getPort();
    
    // Get Host header for virtual hosting (not strictly required)
    std::string host_header = req.getHeader("Host");
    
    // Find the correct server based on Host header (virtual hosting)
    int server_index = state.server_index;  // Default to original
    if (!host_header.empty()) {
        int matched = findServerByHost(host_header, port);
        if (matched != -1) {
            server_index = matched;
        }
    }
    
    Server* server = servers[server_index];
    std::cout << "[" << server->getConfig().server_name << ":" << server->getPort() 
              << "] " << req.getMethod() << " " << req.getPath() << std::endl;
    
    // Get the response from server (don't send directly)
    Response response = server->handleRequest(req);
    
    // Queue the response to be sent when POLLOUT is ready
    queueResponse(client_fd, response.toString());
}

void ServerManager::queueResponse(int client_fd, const std::string& response) {
    std::map<int, ClientState>::iterator it = client_states.find(client_fd);
    if (it == client_states.end()) {
        return;
    }
    
    it->second.response_buffer = response;
    it->second.bytes_sent = 0;
    it->second.response_ready = true;
}

void ServerManager::handleClientWrite(int client_fd) {
    std::map<int, ClientState>::iterator it = client_states.find(client_fd);
    if (it == client_states.end()) {
        closeClient(client_fd);
        return;
    }
    
    ClientState& state = it->second;
    
    // If no response is ready, nothing to write
    if (!state.response_ready || state.response_buffer.empty()) {
        return;
    }
    
    // Calculate remaining data to send
    size_t remaining = state.response_buffer.length() - state.bytes_sent;
    if (remaining == 0) {
        // All data sent, reset for next request (keep-alive)
        state.request.reset();
        state.response_buffer.clear();
        state.bytes_sent = 0;
        state.response_ready = false;
        state.last_activity = time(NULL);
        return;
    }
    
    // Write data to socket (only ONE write per poll cycle)
    const char* data = state.response_buffer.c_str() + state.bytes_sent;
    ssize_t bytes_written = write(client_fd, data, remaining);
    
    // Check return value properly (both -1 and 0)
    if (bytes_written <= 0) {
        // Error or connection closed, remove client
        closeClient(client_fd);
        return;
    }
    
    // Update bytes sent
    state.bytes_sent += bytes_written;
    
    // Check if we've sent everything
    if (state.bytes_sent >= state.response_buffer.length()) {
        // All data sent, reset for next request (keep-alive)
        state.request.reset();
        state.response_buffer.clear();
        state.bytes_sent = 0;
        state.response_ready = false;
        state.last_activity = time(NULL);
    }
}

void ServerManager::updatePollEvents(int fd, short events) {
    for (size_t i = 0; i < poll_fds.size(); i++) {
        if (poll_fds[i].fd == fd) {
            poll_fds[i].events = events;
            break;
        }
    }
}

void ServerManager::closeClient(int client_fd) {
    removePollFd(client_fd);
    fd_to_server.erase(client_fd);
    client_states.erase(client_fd);
    close(client_fd);
}

void ServerManager::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> to_close;
    
    // Find all timed-out connections
    for (std::map<int, ClientState>::iterator it = client_states.begin();
         it != client_states.end(); ++it) {
        if (now - it->second.last_activity > CONNECTION_TIMEOUT) {
            to_close.push_back(it->first);
        }
    }
    
    // Close timed-out connections
    for (size_t i = 0; i < to_close.size(); i++) {
        closeClient(to_close[i]);
    }
}

void ServerManager::addPollFd(int fd, short events) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    poll_fds.push_back(pfd);
}

void ServerManager::removePollFd(int fd) {
    for (size_t i = 0; i < poll_fds.size(); i++) {
        if (poll_fds[i].fd == fd) {
            poll_fds.erase(poll_fds.begin() + i);
            break;
        }
    }
}

void ServerManager::stop() {
    // Close all client connections
    for (size_t i = 0; i < poll_fds.size(); i++) {
        close(poll_fds[i].fd);
    }
    poll_fds.clear();
    fd_to_server.clear();
    client_states.clear();
    
    // Delete all servers
    for (size_t i = 0; i < servers.size(); i++) {
        delete servers[i];
    }
    servers.clear();
}

// Extract hostname from Host header (removes port if present)
// e.g., "localhost:8080" -> "localhost"
std::string ServerManager::extractHostname(const std::string& host) const {
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        return host.substr(0, colon_pos);
    }
    return host;
}

// Find the best matching server based on Host header and port
// Returns server index, or -1 if no match (should use default)
int ServerManager::findServerByHost(const std::string& host, int port) const {
    std::string hostname = extractHostname(host);
    int first_match_on_port = -1;
    
    for (size_t i = 0; i < servers.size(); i++) {
        const ServerConfig& config = servers[i]->getConfig();
        
        // Check if this server listens on the same port
        if (config.port == port) {
            // Remember first server on this port as fallback
            if (first_match_on_port == -1) {
                first_match_on_port = i;
            }
            
            // Check if server_name matches
            if (config.server_name == hostname) {
                return i;
            }
        }
    }
    
    // No exact match, use first server on this port as default
    return first_match_on_port;
}


