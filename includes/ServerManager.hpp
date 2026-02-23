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

#define CONNECTION_TIMEOUT 60
#define CGI_TIMEOUT 30

struct	ClientState
{
	Request		request;
	std::string	response_buffer;
	size_t		bytes_sent;
	int			server_index;
	bool		response_ready;
	time_t		last_activity;
	bool		keep_alive;
	
	bool		cgi_in_progress;
	int			cgi_stdin_fd;
	int			cgi_stdout_fd;
	pid_t		cgi_pid;
	std::string	cgi_input;
	size_t		cgi_input_sent;
	std::string	cgi_output;
	time_t		cgi_start_time;
	CGI*		cgi_handler;
	
	ClientState() : bytes_sent(0), server_index(-1), response_ready(false), last_activity(time(NULL)), keep_alive(true), cgi_in_progress(false), cgi_stdin_fd(-1), cgi_stdout_fd(-1), cgi_pid(-1), cgi_input_sent(0), cgi_start_time(0), cgi_handler(NULL) {}
};

class   ServerManager
{
	private:
		std::vector<Server*>		servers;
		std::vector<struct pollfd>	poll_fds;
		std::map<int, int>			fd_to_server;
		std::set<int>				server_fds;
		std::map<int, ClientState>	client_states;
		std::map<int, int>			cgi_fd_to_client;

		void		addPollFd(int fd, short events);
		void		removePollFd(int fd);
		void		updatePollEvents(int fd, short events);

		void		handleNewConnection(int server_index);
		void		handleClientRequest(int client_fd);
		void		handleClientWrite(int client_fd);

		void		queueResponse(int client_fd, const std::string& response);
		void		closeClient(int client_fd);
		void		checkTimeouts();
		int			findServerByHost(const std::string& host, int port) const;
		std::string	extractHostname(const std::string& host) const;

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