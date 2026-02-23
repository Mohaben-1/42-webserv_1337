#include "ServerManager.hpp"
#include "Response.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sstream>

ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
	stop();
}

bool	ServerManager::initServers(const std::vector<ServerConfig>& configs)
{
	std::map<int, int>	port_to_server_index;

	for (size_t i = 0; i < configs.size(); i++)
	{
		int	port = configs[i].port;

		if (port_to_server_index.find(port) != port_to_server_index.end())
		{
			Server*	server = new Server(configs[i]);
			servers.push_back(server);
			continue ;
		}

		Server*	server = new Server(configs[i]);
		if (!server->start())
		{
			std::cerr << "Failed to start server on port " << port << std::endl;
			delete server;
			return false;
		}
		servers.push_back(server);
		port_to_server_index[port] = i;

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
		int	activity = poll(&poll_fds[0], poll_fds.size(), 1000);
		if (activity < 0)
		{
			std::cerr << "poll() error" << std::endl;
			break ;
		}

		if (time(NULL) - last_timeout_check >= 5)
		{
			checkTimeouts();
			last_timeout_check = time(NULL);
		}

		if (activity == 0)
			continue ;

		for (size_t i = 0; i < poll_fds.size(); i++)
		{
			if (poll_fds[i].revents & POLLIN && server_fds.find(poll_fds[i].fd) != server_fds.end())
				handleNewConnection(fd_to_server[poll_fds[i].fd]);
		}

		for (size_t i = 0; i < poll_fds.size(); i++)
		{
			int		fd = poll_fds[i].fd;
			short	revents = poll_fds[i].revents;

			if (revents == 0 || server_fds.find(fd) != server_fds.end())
				continue ;

			std::map<int, int>::iterator	cgi_it = cgi_fd_to_client.find(fd);
			if (cgi_it != cgi_fd_to_client.end())
			{
				int										client_fd = cgi_it->second;
				std::map<int, ClientState>::iterator	state_it = client_states.find(client_fd);
				if (state_it == client_states.end())
				{
					removePollFd(fd);
					close(fd);
					cgi_fd_to_client.erase(fd);
					i--;
					continue ;
				}

				ClientState&	state = state_it->second;

				if (revents & (POLLERR | POLLNVAL))
				{
					finishCGI(client_fd, false);
					i = static_cast<size_t>(-1);
					continue ;
				}
				if ((revents & POLLIN) && fd == state.cgi_stdout_fd)
				{
					handleCGIRead(fd);
					continue ;
				}
				if ((revents & POLLHUP) && fd == state.cgi_stdout_fd)
				{
					handleCGIRead(fd);
					finishCGI(client_fd, true);
					i = static_cast<size_t>(-1);
					continue ;
				}
				if ((revents & POLLOUT) && fd == state.cgi_stdin_fd)
				{
					handleCGIWrite(fd);
					continue ;
				}
				continue ;
			}

			if (revents & (POLLERR | POLLNVAL))
			{
				closeClient(fd);
				i--;
				continue ;
			}

			if (revents & POLLIN)
			{
				if (client_states.find(fd) != client_states.end())
					handleClientRequest(fd);
			}

			if (client_states.find(fd) == client_states.end())
				continue ;

			if (revents & POLLOUT)
				handleClientWrite(fd);

			if ((revents & POLLHUP) && !(revents & POLLIN))
			{
				if (client_states.find(fd) != client_states.end())
				{
					closeClient(fd);
					i--;
				}
			}
		}
	}
}

void	ServerManager::handleNewConnection(int server_index)
{
	Server*	server = servers[server_index];
	int		server_fd = server->getServerFd();

	while (true)
	{
		int	client_fd = accept(server_fd, NULL, NULL);

		if (client_fd < 0)
			break ;
		fcntl(client_fd, F_SETFL, O_NONBLOCK);

		addPollFd(client_fd, POLLIN);
		fd_to_server[client_fd] = server_index;

		ClientState	state;
		state.server_index = server_index;
		client_states[client_fd] = state;
	}
}

void	ServerManager::handleClientRequest(int client_fd)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
	{
		std::cerr << "No state found for client fd " << client_fd << std::endl;
		closeClient(client_fd);
		return ;
	}

	if (it->second.response_ready)
		return ;

	if (it->second.cgi_in_progress)
		return ;

	char	buffer[8192];
	ssize_t	bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

	if (bytes_read <= 0)
	{
		closeClient(client_fd);
		return ;
	}
	buffer[bytes_read] = '\0';

	ClientState&	state = it->second;

	state.last_activity = time(NULL);

	Request&	req = state.request;


	req.appendData(std::string(buffer, bytes_read));

	if (!req.isHeadersComplete())
	{
		if (!req.parseHeaders())
			return ;

		if (req.hasParseError())
		{
			state.keep_alive = false;
			Response	res;
			int			ec = req.getErrorCode();
			std::string	status_text;
			if (ec == 505)
				status_text = "HTTP Version Not Supported";
			else
				status_text = "Bad Request";
			res.setStatus(ec, status_text);
			res.setHeader("Content-Type", "text/html");
			res.setHeader("Connection", "close");
			std::ostringstream	body;
			body << "<html><body><h1>" << ec << " " << status_text << "</h1></body></html>";
			res.setBody(body.str());
			queueResponse(client_fd, res.toString());
			return ;
		}

		Server*	server = servers[state.server_index];
		size_t	max_size = server->getConfig().client_max_body_size;

		if (req.getContentLength() > max_size)
		{
			state.keep_alive = false;
			Response	res;
			res.setStatus(413, "Payload Too Large");
			res.setHeader("Content-Type", "text/html");
			res.setHeader("Connection", "close");
			res.setBody("<html><body><h1>413 Payload Too Large</h1></body></html>");
			queueResponse(client_fd, res.toString());
			return ;
		}
	}

	if (!req.isComplete())
		return;

	std::string	conn_header = req.getHeader("Connection");

	for (size_t ci = 0; ci < conn_header.length(); ci++)
		conn_header[ci] = tolower(conn_header[ci]);
	if (conn_header == "close")
		state.keep_alive = false;
	else
		state.keep_alive = true;

	Server*	original_server = servers[state.server_index];
	int		port = original_server->getPort();

	std::string	host_header = req.getHeader("Host");

	int	server_index = state.server_index;
	
	if (!host_header.empty())
	{
		int	matched = findServerByHost(host_header, port);

		if (matched != -1)
			server_index = matched;
	}

	Server*	server = servers[server_index];
	std::cout << "[" << server->getConfig().server_name << ":" << server->getPort() << "] " << req.getMethod() << " " << req.getPath() << std::endl;

	CGIInfo	cgi_info;
	if (server->isCGIRequest(req, cgi_info))
	{
		if (!startCGI(client_fd, req, server, cgi_info.location, cgi_info.cgi_extension, cgi_info.interpreter))
		{
			Response	res;
			res.setStatus(500, "Internal Server Error");
			res.setHeader("Content-Type", "text/html");
			res.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI execution failed</p></body></html>");
			queueResponse(client_fd, res.toString());
		}
		return ;
	}

	Response	response = server->handleNonCGIRequest(req);
	if (state.keep_alive)
		response.setHeader("Connection", "keep-alive");
	else
		response.setHeader("Connection", "close");
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

	updatePollEvents(client_fd, POLLIN | POLLOUT);
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

	if (!state.response_ready || state.response_buffer.empty())
		return ;

	size_t	remaining = state.response_buffer.length() - state.bytes_sent;

	if (remaining > 0)
	{
		const char*	data = state.response_buffer.c_str() + state.bytes_sent;
		ssize_t		bytes_written = write(client_fd, data, remaining);

		if (bytes_written <= 0)
		{
			closeClient(client_fd);
			return ;
		}
		state.bytes_sent += bytes_written;
	}

	if (state.bytes_sent >= state.response_buffer.length())
	{
		if (!state.keep_alive)
		{
			closeClient(client_fd);
			return ;
		}
		state.request.reset();
		state.response_buffer.clear();
		state.bytes_sent = 0;
		state.response_ready = false;
		state.last_activity = time(NULL);
		updatePollEvents(client_fd, POLLIN);
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
	cleanupCGI(client_fd);
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

	for (std::map<int, ClientState>::iterator it = client_states.begin(); it != client_states.end(); ++it)
	{
		if (now - it->second.last_activity > CONNECTION_TIMEOUT)
			to_close.push_back(it->first);
		else if (it->second.cgi_in_progress && now - it->second.cgi_start_time > CGI_TIMEOUT)
			cgi_timeout.push_back(it->first);
	}

	for (size_t i = 0; i < cgi_timeout.size(); i++)
	{
		std::cerr << "CGI timeouted after " << CGI_TIMEOUT << " seconds for client " << cgi_timeout[i] << std::endl;
		finishCGI(cgi_timeout[i], false);
	}

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
	for (size_t i = 0; i < poll_fds.size(); i++)
	{
		int fd = poll_fds[i].fd;

		if (server_fds.find(fd) == server_fds.end())
			close(fd);
	}
	poll_fds.clear();
	fd_to_server.clear();
	client_states.clear();

	for (size_t i = 0; i < servers.size(); i++)
		delete servers[i];
	servers.clear();
}

std::string	ServerManager::extractHostname(const std::string& host) const
{
	size_t	colon_pos = host.find(':');

	if (colon_pos != std::string::npos)
		return (host.substr(0, colon_pos));
	return (host);
}

int	ServerManager::findServerByHost(const std::string& host, int port) const
{
	std::string	hostname = extractHostname(host);
	int			first_match_on_port = -1;

	for (size_t i = 0; i < servers.size(); i++)
	{
		const ServerConfig&	config = servers[i]->getConfig();

		if (config.port == port)
		{
			if (first_match_on_port == -1)
				first_match_on_port = i;
			if (config.server_name == hostname)
				return (i);
		}
	}
	return (first_match_on_port);
}

bool	ServerManager::startCGI(int client_fd, const Request& req, Server* server, const LocationConfig* location, const std::string& extension, const std::string& interpreter)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return (false);

	ClientState&	state = it->second;
	std::string		doc_root = server->getConfig().root;
	std::string		url_path = req.getPath();
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
	struct stat	st;
	if (stat(script_path.c_str(), &st) != 0)
		return (false);
	
	CGI*	cgi = new CGI();
	cgi->setupFromRequest(req, script_path, interpreter, doc_root, server->getPort(), server->getServerName());

	int			stdin_fd = -1;
	int			stdout_fd = -1;
	pid_t		pid = -1;	
	CGIStatus	status = cgi->executeCgi(stdin_fd, stdout_fd, pid);
	if (status != CGI_SUCCESS)
	{
		delete cgi;
		return (false);
	}
	
	state.cgi_in_progress = true;
	state.cgi_stdin_fd = stdin_fd;
	state.cgi_stdout_fd = stdout_fd;
	state.cgi_pid = pid;
	state.cgi_input = req.getBody();
	state.cgi_input_sent = 0;
	state.cgi_output.clear();
	state.cgi_start_time = time(NULL);
	state.cgi_handler = cgi;
	addPollFd(stdout_fd, POLLIN);
	cgi_fd_to_client[stdout_fd] = client_fd;

	if (!state.cgi_input.empty())
	{
		addPollFd(stdin_fd, POLLOUT);
		cgi_fd_to_client[stdin_fd] = client_fd;
	}
	else
	{
		close(stdin_fd);
		state.cgi_stdin_fd = -1;
	}
	return (true);
}

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

	size_t	remaining = state.cgi_input.length() - state.cgi_input_sent;
	if (remaining == 0)
	{
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
		return ;
	}

	const char*	data = state.cgi_input.c_str() + state.cgi_input_sent;
	ssize_t		bytes_written = write(cgi_stdin_fd, data, remaining);

	if (bytes_written <= 0)
	{
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
		return ;
	}

	state.cgi_input_sent += bytes_written;
	if (state.cgi_input_sent >= state.cgi_input.length())
	{
		removePollFd(cgi_stdin_fd);
		cgi_fd_to_client.erase(cgi_stdin_fd);
		close(cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
	}
}

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

	char	buffer[4096];
	ssize_t	bytes_read = read(cgi_stdout_fd, buffer, sizeof(buffer) - 1);

	if (bytes_read > 0)
	{
		buffer[bytes_read] = '\0';
		state.cgi_output += buffer;
	}
	else if (bytes_read == 0)
		finishCGI(client_fd, true);
}

void	ServerManager::finishCGI(int client_fd, bool success)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);
	if (it == client_states.end())
		return ;

	ClientState& state = it->second;
	if (!state.cgi_in_progress)
		return ;

	bool	cgi_failed = false;
	if (state.cgi_pid > 0)
	{
		if (!success)
		{
			kill(state.cgi_pid, SIGKILL);
			cgi_failed = true;
		}

		int		child_status;
		pid_t	result = waitpid(state.cgi_pid, &child_status, 0);
		if (result < 0)
			cgi_failed = true;
		else if (WIFSIGNALED(child_status))
			cgi_failed = true;
		else if (WIFEXITED(child_status))
		{
			if (WEXITSTATUS(child_status) != 0)
				cgi_failed = true;
		}
	}
	else
		cgi_failed = true;

	if (!success)
		cgi_failed = true;


	Response	response;

	if (!cgi_failed && state.cgi_handler && !state.cgi_output.empty())
	{
		response = state.cgi_handler->buildResponseFromOutput(state.cgi_output);
		if (response.getStatusCode() == 500)
			cgi_failed = true;
	}
	else
		cgi_failed = true;

	if (cgi_failed)
	{
		response.setStatus(500, "Internal Server Error");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI script failed</p></body></html>");
	}

	if (state.keep_alive)
		response.setHeader("Connection", "keep-alive");
	else
		response.setHeader("Connection", "close");

	queueResponse(client_fd, response.toString());
	cleanupCGI(client_fd);
}

void	ServerManager::cleanupCGI(int client_fd)
{
	std::map<int, ClientState>::iterator	it = client_states.find(client_fd);

	if (it == client_states.end())
		return ;

	ClientState&	state = it->second;

	if (state.cgi_stdout_fd >= 0)
	{
		removePollFd(state.cgi_stdout_fd);
		cgi_fd_to_client.erase(state.cgi_stdout_fd);
		close(state.cgi_stdout_fd);
		state.cgi_stdout_fd = -1;
	}

	if (state.cgi_stdin_fd >= 0)
	{
		removePollFd(state.cgi_stdin_fd);
		cgi_fd_to_client.erase(state.cgi_stdin_fd);
		close(state.cgi_stdin_fd);
		state.cgi_stdin_fd = -1;
	}

	if (state.cgi_handler)
	{
		delete state.cgi_handler;
		state.cgi_handler = NULL;
	}

	state.cgi_in_progress = false;
	state.cgi_pid = -1;
	state.cgi_input.clear();
	state.cgi_input_sent = 0;
	state.cgi_output.clear();
	state.cgi_start_time = 0;
}
