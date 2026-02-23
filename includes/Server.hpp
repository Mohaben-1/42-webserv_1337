#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"

struct	CGIInfo
{
	std::string				interpreter;
	std::string				cgi_extension;
	const LocationConfig*	location;

	CGIInfo() : location(NULL) {}
};

class	Server
{
	private:
		int				server_fd;
		ServerConfig	config;

		const LocationConfig*	findLocation(const std::string& path) const;
		bool					isMethodAllowed(const std::string& method, const LocationConfig* location) const;

		std::string				readFile(const std::string& path);
		bool					fileExists(const std::string& path);
		bool					isDirectory(const std::string& path);
		bool					writeFile(const std::string& path, const std::string& content);

		Response				serveFile(const std::string& path, const LocationConfig* location);
		Response				serveDirectory(const std::string& fs_path, const std::string& uri_path, const LocationConfig* location);
		Response				serveErrorPage(int code, const std::string& message);
		Response				serveRedirect(int code, const std::string& url);
		Response				serve403();
		Response				serve404();
		Response				serve405();
		Response				serve413();
		Response				serve500();
		Response				serve501();

		Response				handlePost(const Request& req, const LocationConfig* location);
		Response				handleMultipartUpload(const Request& req, const LocationConfig* location);
		Response				handleRawUpload(const Request& req, const LocationConfig* location);

		Response				handleDelete(const Request& req, const LocationConfig* location);
		bool					deleteFile(const std::string& path);

		std::string				buildFilePath(const std::string& uri, const LocationConfig* location);
		std::string				getUploadPath(const LocationConfig* location) const;
		std::string				generateFilename() const;
	public:
		Server(const ServerConfig& cfg);
		~Server();

		bool				start();
		void				stop();

		int					getServerFd() const { return server_fd; }

		int					getPort() const { return config.port; }
		std::string			getServerName() const { return config.server_name; }
		const ServerConfig&	getConfig() const { return config; }

		bool				isCGIRequest(const Request& req, CGIInfo& info);

		Response			handleNonCGIRequest(const Request& req);
};

#endif