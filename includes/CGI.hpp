#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include <vector>
#include "Request.hpp"
#include "Response.hpp"

// CGI execution status
enum	CGIStatus
{
	CGI_SUCCESS,
	CGI_ERROR_FORK,
	CGI_ERROR_PIPE,
	CGI_ERROR_EXEC,
	CGI_ERROR_TIMEOUT,
	CGI_ERROR_READ,
	CGI_ERROR_SCRIPT_NOT_FOUND,
	CGI_ERROR_NO_PERMISSION
};

class	CGI
{
	private:
		std::string							script_path;			// Full path to the CGI script
		std::string							cgi_interpreter;		// Path to interpreter (python3, php, etc.)
		std::string							query_string;			// Query string from URL
		std::string							request_method;			// GET, POST
		std::string							content_type;			// Content-Type of request body
		size_t								content_length;			// Content-Length of request body
		std::string							request_body;			// POST body data
		std::string							server_name;			// Server hostname
		int									server_port;			// Server port
		std::string							script_name;			// Script name (URL path)
		std::string							path_info;				// Extra path info after script name
		std::string							document_root;			// Document root
		std::string							remote_addr;			// Client IP address
		std::map<std::string, std::string>	http_headers; // Additional HTTP headers to pass to CGI
		CGIStatus							status;

		char**		buildEnvArray() const;
		void		freeEnvArray(char** env) const;
		std::string	extractPathInfo(const std::string& url, const std::string& script) const;
	public:
		CGI();
		~CGI();
		
		// Configuration
		void				addHttpHeader(const std::string& key, const std::string& value);
		
		// Setup from Request object
		void				setupFromRequest(const Request& req, const std::string& script_path, const std::string& interpreter, const std::string& doc_root, int port, const std::string& server_name = "localhost");
		
		// Non-blocking CGI execution for poll integration
		CGIStatus			executeCgi(int& stdin_fd, int& stdout_fd, pid_t& pid);
		
		// Build response from externally collected output
		Response			buildResponseFromOutput(const std::string& output) const;
		
		// Parse CGI output into response (public for async use)
		bool				parseOutputString(const std::string& output, Response& response) const;
		
		// Static utility
		static bool			isCGIRequest(const std::string& path, const std::string& extension);
		static std::string	getScriptPath(const std::string& url_path, const std::string& document_root, const std::string& cgi_extension);
};

#endif
