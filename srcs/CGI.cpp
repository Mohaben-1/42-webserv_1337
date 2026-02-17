#include "CGI.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sstream>
#include <climits>

CGI::CGI() : content_length(0), server_port(80), status(CGI_SUCCESS) {}

CGI::~CGI() {}

void CGI::addHttpHeader(const std::string& key, const std::string& value)
{
	std::string	cgi_key = "HTTP_";

	for (size_t i = 0; i < key.length(); i++)
	{
		if (key[i] == '-')
			cgi_key += '_';
		else
			cgi_key += toupper(key[i]);
	}
	http_headers[cgi_key] = value;
}

void	CGI::setupFromRequest(const Request& req, const std::string& script,
							const std::string& interpreter, const std::string& doc_root,
							int port, const std::string& srv_name)
{
	script_path = script;
	cgi_interpreter = interpreter;
	document_root = doc_root;
	server_port = port;
	server_name = srv_name;
	
	request_method = req.getMethod();
	content_type = req.getHeader("Content-Type");
	content_length = req.getContentLength();
	
	// Parse URL to get script name and query string
	std::string	url = req.getPath();
	size_t		qmark = url.find('?');

	if (qmark != std::string::npos)
	{
		query_string = url.substr(qmark + 1);
		script_name = url.substr(0, qmark);
	}
	else
		script_name = url;

	
	// Extract PATH_INFO (extra path after script name)
	path_info = extractPathInfo(script_name, script);

	// Add common HTTP headers
	addHttpHeader("Host", req.getHeader("Host"));
	addHttpHeader("User-Agent", req.getHeader("User-Agent"));
	addHttpHeader("Accept", req.getHeader("Accept"));
	addHttpHeader("Accept-Language", req.getHeader("Accept-Language"));
	addHttpHeader("Accept-Encoding", req.getHeader("Accept-Encoding"));
	addHttpHeader("Connection", req.getHeader("Connection"));
	addHttpHeader("Cookie", req.getHeader("Cookie"));
	addHttpHeader("Referer", req.getHeader("Referer"));

	// Remove empty headers
	std::map<std::string, std::string>::iterator	it = http_headers.begin();

	while (it != http_headers.end())
	{
		if (it->second.empty())
		{
			std::map<std::string, std::string>::iterator to_erase = it;
			++it;
			http_headers.erase(to_erase);
		}
		else
			++it;
	}
}

std::string	CGI::extractPathInfo(const std::string& url, const std::string& script) const
{
	// Find where the script name ends in the URL
	// Get just the script filename
	std::string	script_name_only;
	size_t		last_slash = script.find_last_of('/');

	if (last_slash != std::string::npos)
		script_name_only = script.substr(last_slash);
	else
		script_name_only = "/" + script;

	// Find script name in URL
	size_t	script_pos = url.find(script_name_only);

	if (script_pos != std::string::npos)
	{
		size_t	after_script = script_pos + script_name_only.length();

		if (after_script < url.length())
			return (url.substr(after_script));
	}
	return ("");
}

char**	CGI::buildEnvArray() const
{
	std::vector<std::string>	env_vars;

	// Standard CGI environment variables
	env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env_vars.push_back("SERVER_SOFTWARE=Webserv/1.0");
	
	// Required for PHP-CGI (force-cgi-redirect security feature)
	env_vars.push_back("REDIRECT_STATUS=200");
	
	env_vars.push_back("REQUEST_METHOD=" + request_method);
	env_vars.push_back("QUERY_STRING=" + query_string);
	env_vars.push_back("SCRIPT_NAME=" + script_name);
	env_vars.push_back("SCRIPT_FILENAME=" + script_path);
	env_vars.push_back("PATH_INFO=" + path_info);
	env_vars.push_back("PATH_TRANSLATED=" + document_root + path_info);
	env_vars.push_back("DOCUMENT_ROOT=" + document_root);
	env_vars.push_back("SERVER_NAME=" + server_name);
	
	std::ostringstream	port_ss;

	port_ss << server_port;
	env_vars.push_back("SERVER_PORT=" + port_ss.str());
	
	// Content headers for POST
	if (!content_type.empty())
		env_vars.push_back("CONTENT_TYPE=" + content_type);
	if (content_length > 0)
	{
		std::ostringstream	cl_ss;
		cl_ss << content_length;
		env_vars.push_back("CONTENT_LENGTH=" + cl_ss.str());
	}

	// Add HTTP headers
	for (std::map<std::string, std::string>::const_iterator it = http_headers.begin(); it != http_headers.end(); ++it)
		env_vars.push_back(it->first + "=" + it->second);

	// Preserve PATH from parent environment
	const char* path_env = getenv("PATH");
	if (path_env)
		env_vars.push_back(std::string("PATH=") + path_env);

	// Build char** array
	char**	env = new char*[env_vars.size() + 1];

	for (size_t i = 0; i < env_vars.size(); i++)
	{
		env[i] = new char[env_vars[i].length() + 1];
		strcpy(env[i], env_vars[i].c_str());
	}
	env[env_vars.size()] = NULL;
	return (env);
}

void	CGI::freeEnvArray(char** env) const
{
	if (!env)
		return;
	for (int i = 0; env[i] != NULL; i++)
		delete[] env[i];
	delete[] env;
}

CGIStatus	CGI::executeCgi(int& stdin_fd, int& stdout_fd, pid_t& child_pid)
{
	status = CGI_SUCCESS;
	stdin_fd = -1;
	stdout_fd = -1;
	child_pid = -1;
	
	// Check if script exists
	struct stat st;
	if (stat(script_path.c_str(), &st) != 0)
	{
		std::cerr << "CGI Error: Script not found: " << script_path << std::endl;
		status = CGI_ERROR_SCRIPT_NOT_FOUND;
		return (status);
	}
	
	// Check if interpreter exists
	if (!cgi_interpreter.empty() && stat(cgi_interpreter.c_str(), &st) != 0)
	{
		std::cerr << "CGI Error: Interpreter not found: " << cgi_interpreter << std::endl;
		status = CGI_ERROR_SCRIPT_NOT_FOUND;
		return (status);
	}

	int	pipe_in[2];
	int	pipe_out[2];

	if (pipe(pipe_in) == -1)
	{
		std::cerr << "CGI Error: Failed to create input pipe" << std::endl;
		status = CGI_ERROR_PIPE;
		return (status);
	}
	if (pipe(pipe_out) == -1)
	{
		std::cerr << "CGI Error: Failed to create output pipe" << std::endl;
		close(pipe_in[0]);
		close(pipe_in[1]);
		status = CGI_ERROR_PIPE;
		return (status);
	}

	pid_t	pid = fork();
	
	if (pid == -1)
	{
		std::cerr << "CGI Error: Fork failed" << std::endl;
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		status = CGI_ERROR_FORK;
		return (status);
	}
	if (pid == 0)
	{
		// Restore SIGPIPE default for CGI scripts (parent ignores it)
		signal(SIGPIPE, SIG_DFL);

		close(pipe_in[1]);
		close(pipe_out[0]);
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[0]);
		close(pipe_out[1]);

		// Change to script directory for relative path access
		std::string	script_dir = script_path;
		std::string	script_filename = script_path;
		size_t		last_slash = script_dir.find_last_of('/');

		if (last_slash != std::string::npos)
		{
			script_dir = script_dir.substr(0, last_slash);
			script_filename = script_path.substr(last_slash + 1);
			chdir(script_dir.c_str());
		}

		char**	env = buildEnvArray();

		char*	argv[3];
		if (!cgi_interpreter.empty())
		{
			argv[0] = const_cast<char*>(cgi_interpreter.c_str());
			argv[1] = const_cast<char*>(script_filename.c_str());
			argv[2] = NULL;
			execve(cgi_interpreter.c_str(), argv, env);
		}
		else
		{
			std::string exec_path = "./" + script_filename;
			argv[0] = const_cast<char*>(exec_path.c_str());
			argv[1] = NULL;
			execve(exec_path.c_str(), argv, env);
		}
		std::cerr << "CGI Error: execve failed: " << strerror(errno) << std::endl;
		freeEnvArray(env);
		_exit(1);
	}
	close(pipe_in[0]);
	close(pipe_out[1]);

	// Set pipes to non-blocking for poll() integration
	fcntl(pipe_in[1], F_SETFL, O_NONBLOCK);
	fcntl(pipe_out[0], F_SETFL, O_NONBLOCK);

	// Return pipe fds and pid to caller (ServerManager will handle I/O through poll)
	stdin_fd = pipe_in[1];
	stdout_fd = pipe_out[0];
	child_pid = pid;
	return (status);
}

bool	CGI::parseOutputString(const std::string& output, Response& response) const
{
	size_t	header_end = output.find("\r\n\r\n");

	if (header_end == std::string::npos)
	{
		// Try with just \n\n
		header_end = output.find("\n\n");
		if (header_end == std::string::npos)
		{
			// No valid CGI header separator found - invalid CGI output
			return (false);
		}
	}

	// Parse headers
	std::string	headers_section = output.substr(0, header_end);
	std::string	body;

	if (output[header_end] == '\r')
		body = output.substr(header_end + 4);
	else
		body = output.substr(header_end + 2);

	// Parse each header line
	std::istringstream	header_stream(headers_section);
	std::string			line;
	bool				has_content_type = false;
	int					status_code = 200;
	std::string			status_message = "OK";
	
	while (std::getline(header_stream, line))
	{
		// Remove \r if present
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		
		if (line.empty())
			continue;
		
		size_t	colon = line.find(':');
		if (colon != std::string::npos)
		{
			std::string	key = line.substr(0, colon);
			std::string	value = line.substr(colon + 1);

			while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
				value.erase(0, 1);

			// Handle special headers
			if (key == "Status")
			{
				std::istringstream status_stream(value);
				status_stream >> status_code;
				std::getline(status_stream, status_message);
				while (!status_message.empty() && status_message[0] == ' ') {
					status_message.erase(0, 1);
				}
				if (status_message.empty()) {
					status_message = "OK";
				}
			}
			else if (key == "Content-Type" || key == "Content-type")
			{
				has_content_type = true;
				response.setHeader("Content-Type", value);
			}
			else if (key == "Location")
			{
				response.setHeader("Location", value);
				if (status_code == 200) {
					status_code = 302;
					status_message = "Found";
				}
			}
			else
				response.setHeader(key, value);
		}
	}
	if (!has_content_type)
		response.setHeader("Content-Type", "text/html");
	response.setStatus(status_code, status_message);
	response.setBody(body);
	return (true);
}

Response	CGI::buildResponseFromOutput(const std::string& output) const
{
	Response	response;

	if (output.empty())
	{
		response.setStatus(500, "Internal Server Error");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI produced no output</p></body></html>");
		return (response);
	}
	if (!parseOutputString(output, response))
	{
		response.setStatus(500, "Internal Server Error");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI produced invalid output (missing headers)</p></body></html>");
		return (response);
	}
	return (response);
}

bool	CGI::isCGIRequest(const std::string& path, const std::string& extension)
{
	if (extension.empty())
		return (false);
	
	// Check if path ends with the CGI extension
	if (path.length() >= extension.length())
	{
		// Find extension in path (it might be followed by path_info)
		size_t	ext_pos = path.find(extension);

		if (ext_pos != std::string::npos)
		{
			// Make sure it's actually at an extension position (after a filename)
			// and followed by / or end of string
			size_t	after_ext = ext_pos + extension.length();

			if (after_ext == path.length() || path[after_ext] == '/' || path[after_ext] == '?')
				return (true);
		}
	}
	return (false);
}

std::string	CGI::getScriptPath(const std::string& url_path, const std::string& document_root, const std::string& cgi_extension)
{
	// Find where the script name ends (at extension + optional path_info)
	size_t	ext_pos = url_path.find(cgi_extension);
	if (ext_pos == std::string::npos)
		return ("");

	// Script path is everything up to and including the extension
	std::string	script_url = url_path.substr(0, ext_pos + cgi_extension.length());

	// Remove query string if present
	size_t	qmark = script_url.find('?');
	if (qmark != std::string::npos)
		script_url = script_url.substr(0, qmark);
	
	// Build full filesystem path
	std::string	relative_path = document_root + script_url;
	
	// Convert to absolute path (required for PHP-CGI)
	char	abs_path[PATH_MAX];
	if (realpath(relative_path.c_str(), abs_path) != NULL)
		return (std::string(abs_path));
	
	// Fallback to relative path if realpath fails
	return (relative_path);
}
