#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct	LocationConfig
{
	std::string							path;
	std::string							root;
	std::string							index;
	std::vector<std::string>			methods;
	bool								autoindex;
	std::string							upload_store;
	std::map<std::string, std::string>	cgi_handlers;
	size_t								client_max_body_size;
	int									redirect_code;
	std::string							redirect_url;

	LocationConfig() : autoindex(false), client_max_body_size(0), redirect_code(0) {}
};

struct	ServerConfig
{
	int							port;
	std::string					server_name;
	std::string					root;
	std::string					index;
	size_t						client_max_body_size;
	std::map<int, std::string>	error_pages;
	std::vector<LocationConfig>	locations;
	
	ServerConfig() : port(8080), client_max_body_size(1048576) {}
};

class	Config
{
	private:
		std::vector<ServerConfig>	servers;

		std::string					trim(const std::string& str);
		std::vector<std::string>	split(const std::string& str, char delimiter);
		size_t						parseSize(const std::string& size_str);
		bool						isNumber(const std::string& str);
		bool						validatePorts() const;
	public:
		Config();

		bool								parse(const std::string& filename);
		const std::vector<ServerConfig>&	getServers() const { return servers; }
		void								print() const;
};

#endif