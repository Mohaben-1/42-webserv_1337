#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

// Structure to hold CGI info for async execution
struct	CGIInfo
{
	std::string				script_path;
	std::string				interpreter;
	std::string				doc_root;
	std::string				cgi_extension;
	const LocationConfig*	location;

	CGIInfo() : location(NULL) {}
};

class	Server
{
	private:
		int				server_fd;
		ServerConfig	config;

		// Location matching
		const LocationConfig*	findLocation(const std::string& path) const;
		bool					isMethodAllowed(const std::string& method, const LocationConfig* location) const;

		// File operations
		std::string				readFile(const std::string& path);
		bool					fileExists(const std::string& path);
		bool					isDirectory(const std::string& path);
		bool					writeFile(const std::string& path, const std::string& content);
		
		// Response builders
		Response				serveFile(const std::string& path, const LocationConfig* location);
		Response				serveDirectory(const std::string& path, const LocationConfig* location);
		Response				serveErrorPage(int code, const std::string& message);
		Response				serveRedirect(int code, const std::string& url);
		Response				serve200(const std::string& message);
		Response				serve403();
		Response				serve404();
		Response				serve405();
		Response				serve413();
		Response				serve500();
		Response				serve201(const std::string& message);
		
		// POST handling
		Response				handlePost(const Request& req, const LocationConfig* location);
		Response				handleMultipartUpload(const Request& req, const LocationConfig* location);
		Response				handleRawUpload(const Request& req, const LocationConfig* location);
		
		// DELETE handling
		Response				handleDelete(const Request& req, const LocationConfig* location);
		bool					deleteFile(const std::string& path);
		
		// Helper
		std::string				buildFilePath(const std::string& uri, const LocationConfig* location);
		std::string				getUploadPath(const LocationConfig* location) const;
		std::string				generateFilename() const;
	public:
		Server();
		Server(const ServerConfig& cfg);
		~Server();
		
		bool				start();
		void				stop();
		
		// Expose server_fd for poll()
		int					getServerFd() const { return server_fd; }
		
		// Get config info
		int					getPort() const { return config.port; }
		std::string			getServerName() const { return config.server_name; }
		const ServerConfig&	getConfig() const { return config; }
		
		// CGI detection for async handling
		bool				isCGIRequest(const Request& req, CGIInfo& info);
		
		// Handle non-CGI request (excludes CGI processing)
		Response			handleNonCGIRequest(const Request& req);
};

#endif