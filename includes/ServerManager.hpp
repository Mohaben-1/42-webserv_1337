#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <vector>
#include <map>
#include <set>
#include <poll.h>
#include "Server.hpp"
#include "Config.hpp"
#include "Request.hpp"
#include "CGI.hpp"
#include <ctime>

// Connection timeout in seconds (for idle connections)
#define CONNECTION_TIMEOUT 60
#define CGI_TIMEOUT 30

// Tracks the state of a client connection
struct	ClientState
{
	Request		request;
	std::string	response_buffer;	// Buffer for outgoing response
	size_t		bytes_sent;			// How many bytes have been sent
	int			server_index;
	bool		response_ready;
	time_t		last_activity;		// Timestamp of last activity
	
	// CGI state (for non-blocking CGI execution through poll)
	bool		cgi_in_progress;
	int			cgi_stdin_fd;			// Pipe to write POST data to CGI
	int			cgi_stdout_fd;			// Pipe to read CGI output from
	pid_t		cgi_pid;				// CGI process ID
	std::string	cgi_input;				// POST data to send to CGI
	size_t		cgi_input_sent;			// Bytes of POST data already sent
	std::string	cgi_output;				// Collected CGI output
	time_t		cgi_start_time;			// For timeout detection
	CGI*		cgi_handler;			// CGI context for building response
	
	ClientState() : bytes_sent(0), server_index(-1), response_ready(false), 
					last_activity(time(NULL)), cgi_in_progress(false),
					cgi_stdin_fd(-1), cgi_stdout_fd(-1), cgi_pid(-1),
					cgi_input_sent(0), cgi_start_time(0), cgi_handler(NULL) {}
};

class   ServerManager
{
	private:
		std::vector<Server*>		servers;
		std::vector<struct pollfd>	poll_fds;
		std::map<int, int>			fd_to_server;			// Maps fd to server index
		std::set<int>				server_fds;				// Track which fds are server sockets
		std::map<int, ClientState>	client_states;			// Track partial requests for each client
		std::map<int, int>			cgi_fd_to_client;		// Maps CGI pipe fds to client fds
		
		void		addPollFd(int fd, short events);
		void		removePollFd(int fd);
		void		updatePollEvents(int fd, short events);
		void		handleNewConnection(int server_index);
		void		handleClientRequest(int client_fd);
		void		handleClientWrite(int client_fd);
		void		queueResponse(int client_fd, const std::string& response);
		void		closeClient(int client_fd);
		void		checkTimeouts();
		int			findServerByFd(int fd) const;
		int			findServerByHost(const std::string& host, int port) const;
		std::string	extractHostname(const std::string& host) const;
		
		// CGI handling through poll
		bool		startCGI(int client_fd, const Request& req, Server* server, const LocationConfig* location, const std::string& extension, const std::string& interpreter);
		void		handleCGIWrite(int cgi_stdin_fd);
		void		handleCGIRead(int cgi_stdout_fd);
		void		finishCGI(int client_fd, bool success);
		void		cleanupCGI(int client_fd);
	public:
		ServerManager();
		~ServerManager();

		bool	initServers(const std::vector<ServerConfig>& configs);
		void	run();
		void	stop();
};

#endif