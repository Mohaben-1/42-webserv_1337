#include "ServerManager.hpp"
#include "Response.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>

ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
	stop();
}

bool    ServerManager::initServers(const std::vector<ServerConfig>& configs)
{
	// Track which ports have been bound (for virtual hosting support)
	std::map<int, int>  port_to_server_index;
	
	// Create and start each server
	for (size_t i = 0; i < configs.size(); i++)
	{
		int	port = configs[i].port;
		
		// Check if this port is already bound by another server
		if (port_to_server_index.find(port) != port_to_server_index.end())
		{
			// Port already in use - this is virtual hosting
			Server*	server = new Server(configs[i]);
			servers.push_back(server);
			
			// Map to the same fd as the first server on this port
			int	first_server_idx = port_to_server_index[port];
			int	shared_fd = servers[first_server_idx]->getServerFd();

			fd_to_server[shared_fd] = first_server_idx;
			continue ;
		}

		// New port - create and bind
		Server*	server = new Server(configs[i]);

		if (!server->start())
		{
			std::cerr << "Failed to start server on port " << port << std::endl;
			delete server;
			return false;
		}
		servers.push_back(server);
		port_to_server_index[port] = i;

		// Add server socket to poll
		int	server_fd = server->getServerFd();

		addPollFd(server_fd, POLLIN);
		fd_to_server[server_fd] = i;
		server_fds.insert(server_fd);
	}
	std::cout << "Webserv ready - listening on " << servers.size() << " server(s)" << std::endl;
	return (true);
}

void	ServerManager::run()
{
	time_t	last_timeout_check = time(NULL);

	while (true)
	{
		// Wait for activity on any socket (with 1 second timeout for checking idle connections)
		int	activity = poll(&poll_fds[0], poll_fds.size(), 1000);
		
		if (activity < 0)
		{
			std::cerr << "poll() error" << std::endl;
			break ;
		}
		
		// Periodically check for timed-out connections
		if (time(NULL) - last_timeout_check >= 5)
		{
			checkTimeouts();
			last_timeout_check = time(NULL);
		}
		
		// Check each file descriptor for BOTH read and write events
		for (size_t i = 0; i < poll_fds.size(); i++)
		{
			int		fd = poll_fds[i].fd;
			short	revents = poll_fds[i].revents;
			
			// Check if this is a CGI pipe fd
			std::map<int, int>::iterator	cgi_it = cgi_fd_to_client.find(fd);

			if (cgi_it != cgi_fd_to_client.end())
			{
				int										client_fd = cgi_it->second;
				std::map<int, ClientState>::iterator	state_it = client_states.find(client_fd);
				if (state_it == client_states.end())
				{
					// Client gone, cleanup CGI
					removePollFd(fd);
					close(fd);
					cgi_fd_to_client.erase(cgi_it);
					continue ;
				}

				ClientState&	state = state_it->second;

				// Handle CGI pipe errors/hangup
				if (revents & (POLLERR | POLLNVAL))
				{
					finishCGI(client_fd, false);
					continue ;
				}

				// Handle CGI stdout read (POLLIN)
				if ((revents & POLLIN) && fd == state.cgi_stdout_fd)
				{
					handleCGIRead(fd);
					continue ;
				}
				
				// Handle POLLHUP on CGI stdout (CGI process finished)
				if ((revents & POLLHUP) && fd == state.cgi_stdout_fd)
				{
					// Read any remaining data first
					handleCGIRead(fd);
					finishCGI(client_fd, true);
					continue ;
				}
				
				// Handle CGI stdin write (POLLOUT)
				if ((revents & POLLOUT) && fd == state.cgi_stdin_fd)
				{
					handleCGIWrite(fd);
					continue ;
				}
				continue ;
			}

			// Handle errors and hangups on non-CGI fds
			if (revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				if (server_fds.find(fd) == server_fds.end())
				{
					// Client socket error
					closeClient(fd);
					continue;
				}
			}

			// Check for read events (POLLIN)
			if (revents & POLLIN)
			{
				// Check if this is a server socket (new connection)
				if (server_fds.find(fd) != server_fds.end())
					handleNewConnection(fd_to_server[fd]);	// This is a server socket - new connection
				else
					handleClientRequest(fd);				// This is a client socket - handle request
			}

			// Check for write events (POLLOUT)
			if (revents & POLLOUT)
			{
				// Only client sockets should have POLLOUT
				if (server_fds.find(fd) == server_fds.end())
					handleClientWrite(fd);
			}
		}
	}
}

void	ServerManager::handleNewConnection(int server_index)
{
	Server*	server = servers[server_index];
	int		server_fd = server->getServerFd();
	
	// Accept new connection
	int	client_fd = accept(server_fd, NULL, NULL);

	if (client_fd < 0)
	{
		std::cerr << "Accept failed on server " << (server_index + 1) << std::endl;
		return ;
	}

	// Set client socket to non-blocking mode
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(client_fd);
		return ;
	}

	// Add client to poll - register for BOTH read and write events
	addPollFd(client_fd, POLLIN | POLLOUT);
	fd_to_server[client_fd] = server_index;

	// Initialize client state for incremental parsing
	ClientState state;
	state.server_index = server_index;
	client_states[client_fd] = state;
}

void	ServerManager::handleClientRequest(int client_fd)
{
	// Get client state first
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
	{
		std::cerr << "No state found for client fd " << client_fd << std::endl;
		closeClient(client_fd);
		return ;
	}

	// If response is already ready, don't read more (wait for write to complete)
	if (it->second.response_ready)
		return;

	// If CGI is in progress, don't read from client (wait for CGI to complete)
	if (it->second.cgi_in_progress)
		return ;

	// Read available data from socket
	char	buffer[8192];
	ssize_t	bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
	
	// Check return value properly (both -1 and 0)
	if (bytes_read <= 0)
	{
		if (bytes_read == 0)
			closeClient(client_fd);	// Client closed connection
		else
			closeClient(client_fd);	// bytes_read < 0: error occurred	// For non-blocking sockets, would-block is not a real error
		return ;
	}
	buffer[bytes_read] = '\0';

	ClientState&	state = it->second;

	state.last_activity = time(NULL);	// Update activity timestamp

	Request&	req = state.request;

	// Append received data to request
	req.appendData(std::string(buffer, bytes_read));

	// Try to parse headers if not done yet
	if (!req.isHeadersComplete())
	{
		if (!req.parseHeaders())
			return;	// Headers not complete yet, wait for more data

		// Check body size limit early
		Server*	server = servers[state.server_index];
		size_t	max_size = server->getConfig().client_max_body_size;

		if (req.getContentLength() > max_size)
		{
			// Queue 413 response
			Response	res;
			res.setStatus(413, "Payload Too Large");
			res.setHeader("Content-Type", "text/html");
			res.setHeader("Connection", "close");
			res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
			queueResponse(client_fd, res.toString());
			return ;
		}
	}

	// Check if request is complete (headers + full body)
	if (!req.isComplete())
		return;	// Still waiting for body data
	
	// Request is complete, process it

	// Get the original server (based on which port received the connection)
	Server*	original_server = servers[state.server_index];
	int		port = original_server->getPort();
	
	// Get Host header for virtual hosting
	std::string	host_header = req.getHeader("Host");
	
	// Find the correct server based on Host header (virtual hosting)
	int	server_index = state.server_index;  // Default to original
	
	if (!host_header.empty())
	{
		int	matched = findServerByHost(host_header, port);

		if (matched != -1)
			server_index = matched;
	}

	Server*	server = servers[server_index];
	std::cout << "[" << server->getConfig().server_name << ":" << server->getPort() << "] " << req.getMethod() << " " << req.getPath() << std::endl;

	// Check if this is a CGI request
	CGIInfo	cgi_info;

	if (server->isCGIRequest(req, cgi_info))
	{
		// Start CGI execution
		if (!startCGI(client_fd, req, server, cgi_info.location, cgi_info.cgi_extension, cgi_info.interpreter))
		{
			// CGI failed to start, send error response
			Response	res;
			res.setStatus(500, "Internal Server Error");
			res.setHeader("Content-Type", "text/html");
			res.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI execution failed</p></body></html>");
			queueResponse(client_fd, res.toString());
		}
		return ;
	}

	// Non-CGI request: get the response from server
	Response	response = server->handleNonCGIRequest(req);
	
	// Queue the response to be sent when POLLOUT is ready
	queueResponse(client_fd, response.toString());
}

void	ServerManager::queueResponse(int client_fd, const std::string& response)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return ;

	it->second.response_buffer = response;
	it->second.bytes_sent = 0;
	it->second.response_ready = true;
}

void	ServerManager::handleClientWrite(int client_fd)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
	{
		closeClient(client_fd);
		return ;
	}

	ClientState&	state = it->second;

	// If no response is ready, nothing to write
	if (!state.response_ready || state.response_buffer.empty())
		return ;
	
	// Calculate remaining data to send
	size_t	remaining = state.response_buffer.length() - state.bytes_sent;

	if (remaining == 0)
	{
		// All data sent, reset for next request (keep-alive)
		state.request.reset();
		state.response_buffer.clear();
		state.bytes_sent = 0;
		state.response_ready = false;
		state.last_activity = time(NULL);
		return ;
	}
	
	// Write data to socket (only ONE write per poll cycle)
	const char*	data = state.response_buffer.c_str() + state.bytes_sent;
	ssize_t		bytes_written = write(client_fd, data, remaining);

	// Check return value properly (both -1 and 0)
	if (bytes_written <= 0)
	{
		// Error or connection closed, remove client
		closeClient(client_fd);
		return ;
	}

	// Update bytes sent
	state.bytes_sent += bytes_written;

	// Check if we've sent everything
	if (state.bytes_sent >= state.response_buffer.length())
	{
		// All data sent, reset for next request (keep-alive)
		state.request.reset();
		state.response_buffer.clear();
		state.bytes_sent = 0;
		state.response_ready = false;
		state.last_activity = time(NULL);
	}
}

void	ServerManager::updatePollEvents(int fd, short events)
{
	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		if (poll_fds[i].fd == fd)
		{
			poll_fds[i].events = events;
			break ;
		}
	}
}

void	ServerManager::closeClient(int client_fd)
{
	cleanupCGI(client_fd);	// Cleanup any ongoing CGI first
	removePollFd(client_fd);
	fd_to_server.erase(client_fd);
	client_states.erase(client_fd);
	close(client_fd);
}

void	ServerManager::checkTimeouts()
{
	time_t				now = time(NULL);
	std::vector<int>	to_close;
	std::vector<int>	cgi_timeout;
	
	// Find all timed-out connections and CGI processes
	for (std::map<int, ClientState>::iterator it = client_states.begin(); it != client_states.end(); ++it)
	{
		// Check connection timeout
		if (now - it->second.last_activity > CONNECTION_TIMEOUT)
			to_close.push_back(it->first);

		// Check CGI timeout
		else if (it->second.cgi_in_progress && now - it->second.cgi_start_time > CGI_TIMEOUT)
			cgi_timeout.push_back(it->first);
	}

	// Handle CGI timeouts
	for (size_t i = 0; i < cgi_timeout.size(); i++)
	{
		std::cerr << "CGI timeout for client " << cgi_timeout[i] << std::endl;
		finishCGI(cgi_timeout[i], false);
	}

	// Close timed-out connections
	for (size_t i = 0; i < to_close.size(); i++)
		closeClient(to_close[i]);
}

void	ServerManager::addPollFd(int fd, short events)
{
	struct pollfd	pfd;

	pfd.fd = fd;
	pfd.events = events;
	pfd.revents = 0;
	poll_fds.push_back(pfd);
}

void	ServerManager::removePollFd(int fd)
{
	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		if (poll_fds[i].fd == fd)
		{
			poll_fds.erase(poll_fds.begin() + i);
			break ;
		}
	}
}

void	ServerManager::stop()
{
	// Close all client connections
	for (size_t i = 0; i < poll_fds.size(); i++)
		close(poll_fds[i].fd);
	poll_fds.clear();
	fd_to_server.clear();
	client_states.clear();

	// Delete all servers
	for (size_t i = 0; i < servers.size(); i++)
		delete servers[i];
	servers.clear();
}

// Extract hostname from Host header (removes port if present)
std::string	ServerManager::extractHostname(const std::string& host) const
{
	size_t	colon_pos = host.find(':');

	if (colon_pos != std::string::npos)
		return (host.substr(0, colon_pos));
	return (host);
}

// Find the best matching server based on Host header and port
// Returns server index, or -1 if no match (should use default)
int	ServerManager::findServerByHost(const std::string& host, int port) const
{
	std::string	hostname = extractHostname(host);
	int			first_match_on_port = -1;

	for (size_t i = 0; i < servers.size(); i++)
	{
		const ServerConfig&	config = servers[i]->getConfig();

		// Check if this server listens on the same port
		if (config.port == port)
		{
			// Remember first server on this port as fallback
			if (first_match_on_port == -1)
				first_match_on_port = i;

			// Check if server_name matches
			if (config.server_name == hostname)
				return (i);
		}
	}
	// No exact match, use first server on this port as default
	return (first_match_on_port);
}

// Start async CGI execution - returns true if CGI started successfully
bool	ServerManager::startCGI(int client_fd, const Request& req, Server* server, const LocationConfig* location, const std::string& extension, const std::string& interpreter)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return (false);

	ClientState&	state = it->second;

	// Get document root and script path
	std::string	doc_root = server->getConfig().root;
	std::string	url_path = req.getPath();
	
	if (location && !location->root.empty())
	{
		doc_root = location->root;
		if (url_path.find(location->path) == 0)
		{
			url_path = url_path.substr(location->path.length());
			if (url_path.empty() || url_path[0] != '/')
				url_path = "/" + url_path;
		}
	}

	std::string	script_path = CGI::getScriptPath(url_path, doc_root, extension);

	// Check if script exists
	struct stat	st;
	if (stat(script_path.c_str(), &st) != 0)
		return (false);
	
	// Create CGI handler
	CGI*	cgi = new CGI();

	cgi->setupFromRequest(req, script_path, interpreter, doc_root, server->getPort(), server->getServerName());

	// Start CGI execution
	int			stdin_fd = -1;
	int			stdout_fd = -1;
	pid_t		pid = -1;	
	CGIStatus	status = cgi->executeCgi(stdin_fd, stdout_fd, pid);

	if (status != CGI_SUCCESS)
	{
		delete cgi;
		return (false);
	}

	// Store CGI state
	state.cgi_in_progress = true;
	state.cgi_stdin_fd = stdin_fd;
	state.cgi_stdout_fd = stdout_fd;
	state.cgi_pid = pid;
	state.cgi_input = req.getBody();
	state.cgi_input_sent = 0;
	state.cgi_output.clear();
	state.cgi_start_time = time(NULL);
	state.cgi_handler = cgi;

	// Register CGI pipes with poll
	// stdout for reading CGI output
	addPollFd(stdout_fd, POLLIN);
	cgi_fd_to_client[stdout_fd] = client_fd;

	// stdin for writing POST data (only if there's data to write)
	if (!state.cgi_input.empty())
	{
		addPollFd(stdin_fd, POLLOUT);
		cgi_fd_to_client[stdin_fd] = client_fd;
	}
	else
	{
		// No input data, close stdin immediately
		close(stdin_fd);
		state.cgi_stdin_fd = -1;
	}
	return (true);
}

// Handle writing POST data to CGI stdin
void	ServerManager::handleCGIWrite(int cgi_stdin_fd)
{
	std::map<int, int>::iterator	cgi_it = cgi_fd_to_client.find(cgi_stdin_fd);

	if (cgi_it == cgi_fd_to_client.end())
		return ;

	int										client_fd = cgi_it->second;
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return ;

	ClientState&	state = it->second;

	// Calculate remaining data to write
	size_t	remaining = state.cgi_input.length() - state.cgi_input_sent;
	if (remaining == 0)
	{
		// All data sent, close stdin to signal EOF to CGI
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
		return ;
	}

	// Write data (one write per poll cycle)
	const char*	data = state.cgi_input.c_str() + state.cgi_input_sent;
	ssize_t		bytes_written = write(cgi_stdin_fd, data, remaining);

	if (bytes_written <= 0)
	{
		// Error or would-block, close stdin
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
		return ;
	}

	state.cgi_input_sent += bytes_written;

	// Check if all data sent
	if (state.cgi_input_sent >= state.cgi_input.length())
	{
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
	}
}

// Handle reading CGI output
void	ServerManager::handleCGIRead(int cgi_stdout_fd)
{
	std::map<int, int>::iterator	cgi_it = cgi_fd_to_client.find(cgi_stdout_fd);

	if (cgi_it == cgi_fd_to_client.end())
		return ;

	int										client_fd = cgi_it->second;
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return ;

	ClientState&	state = it->second;

	// Read data (one read per poll cycle)
	char	buffer[4096];
	ssize_t	bytes_read = read(cgi_stdout_fd, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0)
	{
		buffer[bytes_read] = '\0';
		state.cgi_output += buffer;
	}
	else if (bytes_read == 0)
		finishCGI(client_fd, true);	// EOF - CGI finished writing
}

// Finish CGI execution and send response
void	ServerManager::finishCGI(int client_fd, bool success)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);
	if (it == client_states.end())
		return ;

	ClientState& state = it->second;

	if (!state.cgi_in_progress)
		return ;
	if (state.cgi_pid > 0)
	{
		int		child_status;
		pid_t	result = waitpid(state.cgi_pid, &child_status, WNOHANG);
		if (result == 0)
		{
			kill(state.cgi_pid, SIGKILL);
			waitpid(state.cgi_pid, &child_status, 0);
		}
	}

	// Build response
	Response	response;

	if (success && state.cgi_handler && !state.cgi_output.empty())
		response = state.cgi_handler->buildResponseFromOutput(state.cgi_output);
	else
	{
		response.setStatus(500, "Internal Server Error");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI execution failed</p></body></html>");
	}

	// Queue response
	queueResponse(client_fd, response.toString());

	// Cleanup CGI state
	cleanupCGI(client_fd);
}

// Cleanup CGI resources
void    ServerManager::cleanupCGI(int client_fd)
{
	std::map<int, ClientState>::iterator    it = client_states.find(client_fd);

	if (it == client_states.end())
		return ;

	ClientState&	state = it->second;

	// Remove and close CGI stdout pipe
	if (state.cgi_stdout_fd >= 0)
	{
		removePollFd(state.cgi_stdout_fd);
		cgi_fd_to_client.erase(state.cgi_stdout_fd);
		close(state.cgi_stdout_fd);
		state.cgi_stdout_fd = -1;
	}
	
	// Remove and close CGI stdin pipe
	if (state.cgi_stdin_fd >= 0)
	{
		removePollFd(state.cgi_stdin_fd);
		cgi_fd_to_client.erase(state.cgi_stdin_fd);
		close(state.cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
	}
	
	// Delete CGI handler
	if (state.cgi_handler)
	{
		delete state.cgi_handler;
		state.cgi_handler = NULL;
	}
	
	// Reset CGI state
	state.cgi_in_progress = false;
	state.cgi_pid = -1;
	state.cgi_input.clear();
	state.cgi_input_sent = 0;
	state.cgi_output.clear();
	state.cgi_start_time = 0;
}
