#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include <sys/types.h>
#include "Request.hpp"
#include "Response.hpp"

enum	CGIStatus
{
	CGI_SUCCESS,
	CGI_ERROR_FORK,
	CGI_ERROR_PIPE,
	CGI_ERROR_SCRIPT_NOT_FOUND
};

class	CGI
{
	private:
		std::string							script_path;
		std::string							cgi_interpreter;
		std::string							query_string;
		std::string							request_method;
		std::string							content_type;
		size_t								content_length;
		std::string							server_name;
		int									server_port;
		std::string							script_name;
		std::string							path_info;
		std::string							document_root;
		std::map<std::string, std::string>	http_headers;
		CGIStatus							status;

		char**		buildEnvArray() const;
		void		freeEnvArray(char** env) const;
		std::string	extractPathInfo(const std::string& url, const std::string& script) const;
		bool		parseOutputString(const std::string& output, Response& response) const;
	public:
		CGI();
		~CGI();

		void				addHttpHeader(const std::string& key, const std::string& value);
		void				setupFromRequest(const Request& req, const std::string& script_path, const std::string& interpreter, const std::string& doc_root, int port, const std::string& server_name = "localhost");
		CGIStatus			executeCgi(int& stdin_fd, int& stdout_fd, pid_t& pid);
		Response			buildResponseFromOutput(const std::string& output) const;
		static bool			isCGIRequest(const std::string& path, const std::string& extension);
		static std::string	getScriptPath(const std::string& url_path, const std::string& document_root, const std::string& cgi_extension);
};

#endif
