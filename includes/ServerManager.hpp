#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include <map>
#include <set>
#include <poll.h>
#include "Server.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include <ctime>

// Connection timeout in seconds (for idle connections)
#define CONNECTION_TIMEOUT 60

// Tracks the state of a client connection
struct ClientState {
    Request request;
    std::string response_buffer;  // Buffer for outgoing response
    size_t bytes_sent;            // How many bytes have been sent
    int server_index;
    bool response_ready;
    time_t last_activity;         // Timestamp of last activity
    
    ClientState() : bytes_sent(0), server_index(-1), response_ready(false), last_activity(time(NULL)) {}
};

class ServerManager {
private:
    std::vector<Server*> servers;
    std::vector<struct pollfd> poll_fds;
    std::map<int, int> fd_to_server;        // Maps fd to server index
    std::set<int> server_fds;               // Track which fds are server sockets
    std::map<int, ClientState> client_states; // Track partial requests for each client
    
public:
    ServerManager();
    ~ServerManager();
    
    bool initServers(const std::vector<ServerConfig>& configs);
    void run();
    void stop();
    
private:
    void addPollFd(int fd, short events);
    void removePollFd(int fd);
    void updatePollEvents(int fd, short events);
    void handleNewConnection(int server_index);
    void handleClientRequest(int client_fd);
    void handleClientWrite(int client_fd);
    void queueResponse(int client_fd, const std::string& response);
    void closeClient(int client_fd);
    void checkTimeouts();
    int findServerByFd(int fd) const;
    int findServerByHost(const std::string& host, int port) const;
    std::string extractHostname(const std::string& host) const;
};

#endif