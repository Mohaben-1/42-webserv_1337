#include "Server.hpp"
#include "CGI.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>

Server::Server(const ServerConfig& cfg) : server_fd(-1), config(cfg) {}

Server::~Server()
{
	stop();
}

bool	Server::start()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		std::cerr << "Error: Failed to create socket" << std::endl;
		return (false);
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error: setsockopt failed" << std::endl;
		return (false);
	}

	if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error: Failed to set non-blocking mode on server socket" << std::endl;
		close(server_fd);
		server_fd = -1;
		return (false);
	}

	struct sockaddr_in	address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(config.port);
	
	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		std::cerr << "Error: Bind failed on port " << config.port << std::endl;
		return (false);
	}

	if (listen(server_fd, SOMAXCONN) < 0)
	{
		std::cerr << "Error: Listen failed" << std::endl;
		return (false);
	}
	std::cout << "[Server] " << config.server_name << ":" << config.port << " started" << std::endl;
	return (true);
}

void	Server::stop()
{
	if (server_fd >= 0)
	{
		close(server_fd);
		server_fd = -1;
	}
}

const LocationConfig*	Server::findLocation(const std::string& path) const
{
	const LocationConfig*	best_match = NULL;
	size_t					best_match_len = 0;

	for (size_t i = 0; i < config.locations.size(); i++)
	{
		const LocationConfig&	loc = config.locations[i];

		if (path.find(loc.path) == 0)
		{
			size_t	loc_len = loc.path.length();

			if (loc_len > best_match_len)
			{
				best_match = &loc;
				best_match_len = loc_len;
			}
		}
	}
	return (best_match);
}

bool	Server::isMethodAllowed(const std::string& method, const LocationConfig* location) const
{
	if (!location)
		return (method == "GET");
	if (location->methods.empty())
		return (true);
	for (size_t i = 0; i < location->methods.size(); i++)
	{
		if (location->methods[i] == method)
			return (true);
	}
	return (false);
}

std::string	Server::buildFilePath(const std::string& uri, const LocationConfig* location)
{
	std::string	base_path = config.root;

	if (location && !location->root.empty())
	{
		base_path = location->root;

		std::string	relative_path = uri;

		if (relative_path.find(location->path) == 0)
			relative_path = relative_path.substr(location->path.length());
		if (relative_path.empty() || relative_path[0] != '/')
			relative_path = "/" + relative_path;
		return (base_path + relative_path);
	}

	std::string	full_path = base_path + uri;
	return (full_path);
}

Response	Server::serveFile(const std::string& path, const LocationConfig* location)
{
	(void)location;

	if (access(path.c_str(), R_OK) != 0)
		return (serve403());

	Response	res;
	std::string	content = readFile(path);
	if (content.empty())
	{
		struct stat	st;
		if (stat(path.c_str(), &st) == 0 && st.st_size > 0)
			return (serve500());
	}
	res.setStatus(200, "OK");
	res.setHeader("Content-Type", Response::getContentType(path));
	res.setBody(content);
	return (res);
}

Response	Server::serveDirectory(const std::string& fs_path, const std::string& uri_path, const LocationConfig* location)
{
	std::string	index_path = fs_path;

	if (index_path[index_path.length() - 1] != '/')
		index_path += "/";

	std::string	index_file = config.index;

	if (location && !location->index.empty())
		index_file = location->index;
	index_path += index_file;
	if (fileExists(index_path))
		return (serveFile(index_path, location));

	if (location && location->autoindex)
	{
		Response	res;

		res.setStatus(200, "OK");
		res.setHeader("Content-Type", "text/html");

		std::string	base = uri_path;
		if (!base.empty() && base[base.size() - 1] != '/')
			base += '/';

		std::ostringstream	html;

		html << "<!DOCTYPE html>\n"
			 << "<html lang=\"en\">\n"
			 << "<head>\n"
			 << "    <meta charset=\"UTF-8\">\n"
			 << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
			 << "    <title>Files • " << uri_path << " • 42 Webserv</title>\n"
			 << "    <style>\n"
			 << "        @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap');\n"
			 << "        \n"
			 << "        :root {\n"
			 << "            --primary: #4f46e5;\n"
			 << "            --primary-dark: #4338ca;\n"
			 << "            --secondary: #10b981;\n"
			 << "            --dark: #1f2937;\n"
			 << "            --light: #f9fafb;\n"
			 << "            --gray: #6b7280;\n"
			 << "            --border: #e5e7eb;\n"
			 << "            --hover-bg: #f3f4f6;\n"
			 << "        }\n"
			 << "        \n"
			 << "        * {\n"
			 << "            margin: 0;\n"
			 << "            padding: 0;\n"
			 << "            box-sizing: border-box;\n"
			 << "        }\n"
			 << "        \n"
			 << "        body {\n"
			 << "            font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;\n"
			 << "            background: linear-gradient(135deg, #f8fafc 0%, #e2e8f0 100%);\n"
			 << "            color: var(--dark);\n"
			 << "            line-height: 1.6;\n"
			 << "            min-height: 100vh;\n"
			 << "            padding: 40px 20px;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .container {\n"
			 << "            max-width: 1000px;\n"
			 << "            margin: 0 auto;\n"
			 << "            background: white;\n"
			 << "            border-radius: 24px;\n"
			 << "            box-shadow: 0 20px 60px rgba(0, 0, 0, 0.08);\n"
			 << "            overflow: hidden;\n"
			 << "            animation: fadeInUp 0.6s ease-out;\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Header */\n"
			 << "        .header {\n"
			 << "            background: white;\n"
			 << "            padding: 30px 40px;\n"
			 << "            border-bottom: 1px solid var(--border);\n"
			 << "            position: relative;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .header::before {\n"
			 << "            content: '';\n"
			 << "            position: absolute;\n"
			 << "            top: 0;\n"
			 << "            left: 0;\n"
			 << "            right: 0;\n"
			 << "            height: 4px;\n"
			 << "            background: linear-gradient(90deg, var(--primary), var(--secondary));\n"
			 << "        }\n"
			 << "        \n"
			 << "        .breadcrumb {\n"
			 << "            display: flex;\n"
			 << "            align-items: center;\n"
			 << "            gap: 8px;\n"
			 << "            flex-wrap: wrap;\n"
			 << "            margin-bottom: 15px;\n"
			 << "            font-family: 'JetBrains Mono', monospace;\n"
			 << "            font-size: 0.9rem;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .breadcrumb a {\n"
			 << "            color: var(--primary);\n"
			 << "            text-decoration: none;\n"
			 << "            transition: color 0.2s;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .breadcrumb a:hover {\n"
			 << "            color: var(--primary-dark);\n"
			 << "            text-decoration: underline;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .breadcrumb .separator {\n"
			 << "            color: var(--gray);\n"
			 << "            font-size: 14px;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .header h1 {\n"
			 << "            font-size: 2rem;\n"
			 << "            font-weight: 600;\n"
			 << "            background: linear-gradient(135deg, var(--dark) 0%, var(--primary) 100%);\n"
			 << "            -webkit-background-clip: text;\n"
			 << "            background-clip: text;\n"
			 << "            color: transparent;\n"
			 << "            word-break: break-all;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .path-badge {\n"
			 << "            display: inline-block;\n"
			 << "            background: var(--light);\n"
			 << "            padding: 6px 15px;\n"
			 << "            border-radius: 20px;\n"
			 << "            font-size: 0.85rem;\n"
			 << "            color: var(--gray);\n"
			 << "            margin-top: 10px;\n"
			 << "            font-family: 'JetBrains Mono', monospace;\n"
			 << "            border: 1px solid var(--border);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .path-badge span {\n"
			 << "            color: var(--primary);\n"
			 << "            font-weight: 500;\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Stats Bar */\n"
			 << "        .stats-bar {\n"
			 << "            background: var(--light);\n"
			 << "            padding: 15px 40px;\n"
			 << "            display: flex;\n"
			 << "            justify-content: space-between;\n"
			 << "            align-items: center;\n"
			 << "            flex-wrap: wrap;\n"
			 << "            gap: 15px;\n"
			 << "            border-bottom: 1px solid var(--border);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .stats-left {\n"
			 << "            display: flex;\n"
			 << "            align-items: center;\n"
			 << "            gap: 20px;\n"
			 << "            flex-wrap: wrap;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .stat-item {\n"
			 << "            display: flex;\n"
			 << "            align-items: center;\n"
			 << "            gap: 8px;\n"
			 << "            color: var(--gray);\n"
			 << "            font-size: 0.9rem;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .stat-value {\n"
			 << "            background: white;\n"
			 << "            padding: 4px 12px;\n"
			 << "            border-radius: 30px;\n"
			 << "            font-weight: 600;\n"
			 << "            color: var(--primary);\n"
			 << "            border: 1px solid var(--border);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .home-link {\n"
			 << "            display: inline-flex;\n"
			 << "            align-items: center;\n"
			 << "            gap: 6px;\n"
			 << "            padding: 8px 16px;\n"
			 << "            background: white;\n"
			 << "            border-radius: 30px;\n"
			 << "            text-decoration: none;\n"
			 << "            color: var(--primary);\n"
			 << "            border: 1px solid var(--border);\n"
			 << "            font-size: 0.9rem;\n"
			 << "            font-weight: 500;\n"
			 << "            transition: all 0.2s;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .home-link:hover {\n"
			 << "            background: var(--primary);\n"
			 << "            color: white;\n"
			 << "            border-color: var(--primary);\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* File List */\n"
			 << "        .file-list {\n"
			 << "            padding: 20px 30px;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .list-header {\n"
			 << "            display: grid;\n"
			 << "            grid-template-columns: 1fr 100px 80px;\n"
			 << "            padding: 15px 20px;\n"
			 << "            background: var(--light);\n"
			 << "            border-radius: 12px;\n"
			 << "            color: var(--gray);\n"
			 << "            font-weight: 600;\n"
			 << "            font-size: 0.8rem;\n"
			 << "            text-transform: uppercase;\n"
			 << "            letter-spacing: 0.5px;\n"
			 << "            border: 1px solid var(--border);\n"
			 << "            margin-bottom: 10px;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .list-item {\n"
			 << "            display: grid;\n"
			 << "            grid-template-columns: 1fr 100px 80px;\n"
			 << "            padding: 15px 20px;\n"
			 << "            border-radius: 10px;\n"
			 << "            transition: all 0.2s ease;\n"
			 << "            align-items: center;\n"
			 << "            border-left: 3px solid transparent;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .list-item:hover {\n"
			 << "            background: var(--hover-bg);\n"
			 << "            border-left-color: var(--primary);\n"
			 << "            transform: translateX(5px);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .item-name {\n"
			 << "            display: flex;\n"
			 << "            align-items: center;\n"
			 << "            gap: 12px;\n"
			 << "            overflow: hidden;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .item-icon {\n"
			 << "            width: 24px;\n"
			 << "            height: 24px;\n"
			 << "            display: flex;\n"
			 << "            align-items: center;\n"
			 << "            justify-content: center;\n"
			 << "            color: var(--gray);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .item-name a {\n"
			 << "            color: var(--dark);\n"
			 << "            text-decoration: none;\n"
			 << "            font-weight: 500;\n"
			 << "            word-break: break-all;\n"
			 << "            transition: color 0.2s;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .item-name a:hover {\n"
			 << "            color: var(--primary);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .directory .item-name a {\n"
			 << "            color: var(--primary);\n"
			 << "            font-weight: 600;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .item-size, .item-date {\n"
			 << "            color: var(--gray);\n"
			 << "            font-size: 0.85rem;\n"
			 << "            font-family: 'JetBrains Mono', monospace;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .parent-item {\n"
			 << "            background: var(--light);\n"
			 << "            margin-bottom: 15px;\n"
			 << "            border-radius: 10px;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .parent-item .list-item {\n"
			 << "            background: var(--light);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .parent-item .item-name a {\n"
			 << "            color: var(--primary);\n"
			 << "            font-weight: 600;\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Empty State */\n"
			 << "        .empty-state {\n"
			 << "            text-align: center;\n"
			 << "            padding: 80px 40px;\n"
			 << "            color: var(--gray);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .empty-state svg {\n"
			 << "            width: 64px;\n"
			 << "            height: 64px;\n"
			 << "            margin-bottom: 20px;\n"
			 << "            opacity: 0.5;\n"
			 << "        }\n"
			 << "        \n"
			 << "        .empty-state h3 {\n"
			 << "            font-size: 1.5rem;\n"
			 << "            margin-bottom: 10px;\n"
			 << "            color: var(--dark);\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Footer */\n"
			 << "        .footer {\n"
			 << "            background: var(--light);\n"
			 << "            padding: 20px 40px;\n"
			 << "            text-align: center;\n"
			 << "            color: var(--gray);\n"
			 << "            font-size: 0.9rem;\n"
			 << "            border-top: 1px solid var(--border);\n"
			 << "        }\n"
			 << "        \n"
			 << "        .footer .server-info {\n"
			 << "            font-family: 'JetBrains Mono', monospace;\n"
			 << "            font-size: 0.8rem;\n"
			 << "            margin-top: 5px;\n"
			 << "            color: var(--primary);\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Animations */\n"
			 << "        @keyframes fadeInUp {\n"
			 << "            from {\n"
			 << "                opacity: 0;\n"
			 << "                transform: translateY(20px);\n"
			 << "            }\n"
			 << "            to {\n"
			 << "                opacity: 1;\n"
			 << "                transform: translateY(0);\n"
			 << "            }\n"
			 << "        }\n"
			 << "        \n"
			 << "        /* Responsive */\n"
			 << "        @media (max-width: 768px) {\n"
			 << "            body {\n"
			 << "                padding: 20px 10px;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .header {\n"
			 << "                padding: 20px;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .header h1 {\n"
			 << "                font-size: 1.5rem;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .stats-bar {\n"
			 << "                padding: 15px 20px;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .file-list {\n"
			 << "                padding: 15px;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .list-header {\n"
			 << "                display: none;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .list-item {\n"
			 << "                grid-template-columns: 1fr;\n"
			 << "                gap: 5px;\n"
			 << "                border: 1px solid var(--border);\n"
			 << "                margin-bottom: 10px;\n"
			 << "                background: white;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .item-size, .item-date {\n"
			 << "                padding-left: 36px;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .item-size::before {\n"
			 << "                content: 'Size: ';\n"
			 << "                font-weight: 600;\n"
			 << "                color: var(--dark);\n"
			 << "            }\n"
			 << "            \n"
			 << "            .item-date::before {\n"
			 << "                content: 'Modified: ';\n"
			 << "                font-weight: 600;\n"
			 << "                color: var(--dark);\n"
			 << "            }\n"
			 << "            \n"
			 << "            .stats-left {\n"
			 << "                flex-direction: column;\n"
			 << "                align-items: flex-start;\n"
			 << "                gap: 10px;\n"
			 << "                width: 100%;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .stat-item {\n"
			 << "                width: 100%;\n"
			 << "                justify-content: space-between;\n"
			 << "            }\n"
			 << "            \n"
			 << "            .home-link {\n"
			 << "                width: 100%;\n"
			 << "                justify-content: center;\n"
			 << "            }\n"
			 << "        }\n"
			 << "    </style>\n"
			 << "</head>\n"
			 << "<body>\n"
			 << "    <div class=\"container\">\n"
			 << "        <div class=\"header\">\n"
			 << "            <div class=\"breadcrumb\">\n"
			 << "                <a href=\"/\">🏠 Home</a>\n"
			 << "                <span class=\"separator\">›</span>\n";

		std::string path_so_far = "/";
		std::string remaining_path = uri_path;
		if (remaining_path[0] == '/')
			remaining_path = remaining_path.substr(1);
		
		size_t pos = 0;
		while ((pos = remaining_path.find('/')) != std::string::npos)
		{
			std::string segment = remaining_path.substr(0, pos);
			if (!segment.empty())
			{
				path_so_far += segment + "/";
				html << "                <a href=\"" << path_so_far << "\">" << segment << "</a>\n";
				html << "                <span class=\"separator\">›</span>\n";
			}
			remaining_path.erase(0, pos + 1);
		}
		if (!remaining_path.empty())
		{
			path_so_far += remaining_path;
			html << "                <span style=\"color: var(--gray);\">" << remaining_path << "</span>\n";
		}
		
		html << "            </div>\n"
			 << "            \n"
			 << "            <h1>📁 ";

		std::string dir_name = uri_path;
		if (dir_name == "/")
			html << "Root Directory";
		else
		{
			if (dir_name[dir_name.length() - 1] == '/')
				dir_name = dir_name.substr(0, dir_name.length() - 1);
			size_t last_slash = dir_name.find_last_of('/');
			if (last_slash != std::string::npos)
				dir_name = dir_name.substr(last_slash + 1);
			html << dir_name;
		}
		
		html << "</h1>\n"
			 << "            \n"
			 << "            <div class=\"path-badge\">\n"
			 << "                <span>Path:</span> " << uri_path << "\n"
			 << "            </div>\n"
			 << "        </div>\n"
			 << "        \n"
			 << "        <div class=\"stats-bar\">\n"
			 << "            <div class=\"stats-left\">\n";

		int dirCount = 0;
		int fileCount = 0;
		DIR* dir = opendir(fs_path.c_str());
		if (dir)
		{
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL)
			{
				std::string name = entry->d_name;
				if (name != "." && name != "..")
				{
					if (entry->d_type == DT_DIR)
						dirCount++;
					else
						fileCount++;
				}
			}
			closedir(dir);
		}
		
		html << "                <div class=\"stat-item\">\n"
			 << "                    <span>📁 Directories</span>\n"
			 << "                    <span class=\"stat-value\">" << dirCount << "</span>\n"
			 << "                </div>\n"
			 << "                <div class=\"stat-item\">\n"
			 << "                    <span>📄 Files</span>\n"
			 << "                    <span class=\"stat-value\">" << fileCount << "</span>\n"
			 << "                </div>\n"
			 << "            </div>\n"
			 << "            \n"
			 << "            <a href=\"/\" class=\"home-link\">\n"
			 << "                <span>🏠</span>\n"
			 << "                Back to Home\n"
			 << "            </a>\n"
			 << "        </div>\n"
			 << "        \n"
			 << "        <div class=\"file-list\">\n";

		if (uri_path != "/")
		{
			html << "            <div class=\"parent-item\">\n"
				 << "                <div class=\"list-item\">\n"
				 << "                    <div class=\"item-name\">\n"
				 << "                        <span class=\"item-icon\">📂</span>\n"
				 << "                        <a href=\"../\">Parent Directory</a>\n"
				 << "                    </div>\n"
				 << "                    <div class=\"item-size\">-</div>\n"
				 << "                    <div class=\"item-date\">-</div>\n"
				 << "                </div>\n"
				 << "            </div>\n";
		}

		html << "            <div class=\"list-header\">\n"
			 << "                <div>Name</div>\n"
			 << "                <div>Size</div>\n"
			 << "                <div>Modified</div>\n"
			 << "            </div>\n";

		dir = opendir(fs_path.c_str());
		if (dir)
		{
			struct dirent* entry;
			std::vector<std::string> directories;
			std::vector<std::string> files;

			while ((entry = readdir(dir)) != NULL)
			{
				std::string name = entry->d_name;
				if (name == "." || name == "..")
					continue;
				
				if (entry->d_type == DT_DIR)
					directories.push_back(name);
				else
					files.push_back(name);
			}
			closedir(dir);

			std::sort(directories.begin(), directories.end());
			std::sort(files.begin(), files.end());

			for (std::vector<std::string>::const_iterator it = directories.begin(); it != directories.end(); ++it)
			{
				const std::string& name = *it;
				std::string href = base + name + "/";
				html << "            <div class=\"list-item directory\">\n"
					 << "                <div class=\"item-name\">\n"
					 << "                    <span class=\"item-icon\">📁</span>\n"
					 << "                    <a href=\"" << href << "\">" << name << "</a>\n"
					 << "                </div>\n"
					 << "                <div class=\"item-size\">-</div>\n"
					 << "                <div class=\"item-date\">-</div>\n"
					 << "            </div>\n";
			}

			for (std::vector<std::string>::const_iterator it = files.begin(); it != files.end(); ++it)
			{
				const std::string& name = *it;
				std::string href = base + name;

				std::string icon = "📄";
				size_t dot_pos = name.find_last_of('.');
				if (dot_pos != std::string::npos)
				{
					std::string ext = name.substr(dot_pos + 1);
					if (ext == "html" || ext == "htm") icon = "🌐";
					else if (ext == "css") icon = "🎨";
					else if (ext == "js") icon = "⚡";
					else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif") icon = "🖼️";
					else if (ext == "pdf") icon = "📕";
					else if (ext == "txt" || ext == "md") icon = "📝";
					else if (ext == "zip" || ext == "tar" || ext == "gz") icon = "📦";
					else if (ext == "mp3" || ext == "wav") icon = "🎵";
					else if (ext == "mp4" || ext == "avi") icon = "🎬";
					else if (ext == "cpp" || ext == "h" || ext == "c") icon = "⚙️";
					else if (ext == "py") icon = "🐍";
					else if (ext == "php") icon = "🐘";
				}
				
				html << "            <div class=\"list-item\">\n"
					 << "                <div class=\"item-name\">\n"
					 << "                    <span class=\"item-icon\">" << icon << "</span>\n"
					 << "                    <a href=\"" << href << "\">" << name << "</a>\n"
					 << "                </div>\n"
					 << "                <div class=\"item-size\">-</div>\n"
					 << "                <div class=\"item-date\">-</div>\n"
					 << "            </div>\n";
			}
		}
		else
		{
			html << "            <div class=\"empty-state\">\n"
				 << "                <svg xmlns=\"http://www.w3.org/2000/svg\" fill=\"none\" viewBox=\"0 0 24 24\" stroke=\"currentColor\">\n"
				 << "                    <path stroke-linecap=\"round\" stroke-linejoin=\"round\" stroke-width=\"2\" d=\"M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z\" />\n"
				 << "                </svg>\n"
				 << "                <h3>Unable to read directory</h3>\n"
				 << "                <p>The directory exists but cannot be accessed.</p>\n"
				 << "            </div>\n";
		}

		html << "        </div>\n"
			 << "        \n"
			 << "        <div class=\"footer\">\n"
			 << "            <div>42 Webserv • File Browser</div>\n"
			 << "            <div class=\"server-info\">\n"
			 << "                HTTP/1.1 • I/O Multiplexing • Static Files • Directory Listing\n"
			 << "            </div>\n"
			 << "        </div>\n"
			 << "    </div>\n"
			 << "</body>\n"
			 << "</html>";

		res.setBody(html.str());
		return (res);
	}
	return (serve404());
}

Response	Server::serveErrorPage(int code, const std::string& message)
{
	Response	res;

	res.setStatus(code, message);
	res.setHeader("Content-Type", "text/html");

	std::map<int, std::string>::const_iterator	it = config.error_pages.find(code);

	if (it != config.error_pages.end())
	{
		std::string	error_page_path = config.root + it->second;

		if (fileExists(error_page_path))
		{
			std::string	content = readFile(error_page_path);

			res.setBody(content);
			return (res);
		}
	}

	std::ostringstream	html;

	html << "<!DOCTYPE html>\n";
	html << "<html>\n<head>\n";
	html << "<title>" << code << " " << message << "</title>\n";
	html << "</head>\n<body>\n";
	html << "<h1>" << code << " " << message << "</h1>\n";
	html << "</body>\n</html>";
	res.setBody(html.str());
	return (res);
}

Response	Server::serve404()
{
	return (serveErrorPage(404, "Not Found"));
}

Response	Server::serve403()
{
	return (serveErrorPage(403, "Forbidden"));
}

Response	Server::serve405()
{
	return (serveErrorPage(405, "Method Not Allowed"));
}

Response	Server::serve500()
{
	return (serveErrorPage(500, "Internal Server Error"));
}

Response	Server::serve501()
{
	return (serveErrorPage(501, "Not Implemented"));
}

Response	Server::serveRedirect(int code, const std::string& url)
{
	Response	res;
	std::string	message;

	switch (code)
	{
		case 301: message = "Moved Permanently"; break ;
		case 302: message = "Found"; break ;
		case 303: message = "See Other"; break ;
		case 307: message = "Temporary Redirect"; break ;
		case 308: message = "Permanent Redirect"; break ;
		default: message = "Redirect"; break ;
	}
	res.setStatus(code, message);
	res.setHeader("Location", url);
	res.setHeader("Content-Type", "text/html");

	std::ostringstream	html;
	html << "<!DOCTYPE html>\n<html>\n<head>\n";
	html << "<title>" << code << " " << message << "</title>\n";
	html << "</head>\n<body>\n";
	html << "<h1>" << code << " " << message << "</h1>\n";
	html << "<p>Redirecting to <a href=\"" << url << "\">" << url << "</a></p>\n";
	html << "</body>\n</html>";
	res.setBody(html.str());
	return (res);
}

std::string	Server::readFile(const std::string& path)
{
	std::ifstream	file(path.c_str(), std::ios::binary);

	if (!file.is_open())
		return ("");

	std::stringstream	buffer;
	buffer << file.rdbuf();
	file.close();
	return (buffer.str());
}

bool	Server::fileExists(const std::string& path)
{
	struct stat	buffer;

	return (stat(path.c_str(), &buffer) == 0);
}

bool	Server::isDirectory(const std::string& path)
{
	struct stat	buffer;

	if (stat(path.c_str(), &buffer) != 0)
		return (false);
	return (S_ISDIR(buffer.st_mode));
}

bool	Server::writeFile(const std::string& path, const std::string& content)
{
	std::ofstream	file(path.c_str(), std::ios::binary);

	if (!file.is_open())
		return (false);
	file.write(content.c_str(), content.length());
	file.close();
	return (file.good());
}

Response	Server::serve413()
{
	return (serveErrorPage(413, "Payload Too Large"));
}

std::string	Server::getUploadPath(const LocationConfig* location) const
{
	if (location && !location->upload_store.empty())
		return (location->upload_store);
	return (config.root + "/uploads");
}

std::string	Server::generateFilename() const
{
	std::ostringstream	oss;

	oss << "upload_" << std::time(NULL) << "_" << (std::rand() % 10000);
	return (oss.str());
}

bool	Server::isCGIRequest(const Request& req, CGIInfo& info)
{
	const LocationConfig*	location = findLocation(req.getPath());

	if (!location || location->cgi_handlers.empty())
		return (false);

	std::string	path = req.getPath();
	size_t		query_pos = path.find('?');
	if (query_pos != std::string::npos)
		path = path.substr(0, query_pos);
	for (std::map<std::string, std::string>::const_iterator it = location->cgi_handlers.begin(); it != location->cgi_handlers.end(); ++it)
	{
		if (CGI::isCGIRequest(path, it->first))
		{
			info.cgi_extension = it->first;
			info.interpreter = it->second;
			info.location = location;
			return (true);
		}
	}
	return (false);
}

Response	Server::handleNonCGIRequest(const Request& req)
{
	const LocationConfig*	location = findLocation(req.getPath());

	if (location && location->redirect_code > 0 && !location->redirect_url.empty())
		return (serveRedirect(location->redirect_code, location->redirect_url));

	std::string	m = req.getMethod();
	if (m != "GET" && m != "POST" && m != "DELETE")
		return (serve501());
	if (!isMethodAllowed(req.getMethod(), location))
		return (serve405());
	if (req.getMethod() == "POST")
	{
		size_t	max_size = config.client_max_body_size;

		if (location && location->client_max_body_size > 0)
			max_size = location->client_max_body_size;
		if (req.getContentLength() > max_size)
			return (serve413());
	}
	if (req.getMethod() == "POST")
		return (handlePost(req, location));
	if (req.getMethod() == "DELETE")
		return (handleDelete(req, location));

	std::string	file_path = buildFilePath(req.getPath(), location);

	if (!fileExists(file_path))
		return (serve404());
	if (isDirectory(file_path))
	{
		std::string	uri = req.getPath();
		if (!uri.empty() && uri[uri.size() - 1] != '/')
			return (serveRedirect(301, uri + "/"));
		return (serveDirectory(file_path, req.getPath(), location));
	}
	return (serveFile(file_path, location));
}

Response	Server::handlePost(const Request& req, const LocationConfig* location)
{
	if (req.isMultipart())
		return (handleMultipartUpload(req, location));
	return (handleRawUpload(req, location));
}

Response	Server::handleMultipartUpload(const Request& req, const LocationConfig* location)
{
	Request&	mutable_req = const_cast<Request&>(req);

	if (!mutable_req.parseMultipart())
	{
		Response	res;
		res.setStatus(400, "Bad Request");
		res.setHeader("Content-Type", "text/html");
		res.setBody("<html><body><h1>400 Bad Request</h1><p>Invalid multipart form data.</p></body></html>");
		return (res);
	}

	const std::vector<MultipartPart>&	parts = req.getParts();
	std::string							upload_dir = getUploadPath(location);

	mkdir(upload_dir.c_str(), 0755);

	int							files_saved = 0;
	std::vector<std::string>	saved_files;
	
	for (size_t i = 0; i < parts.size(); i++)
	{
		const MultipartPart&	part = parts[i];

		if (!part.is_file || part.filename.empty())
			continue ;

		std::string	base_path = upload_dir + "/" + part.filename;
		std::string	file_path = base_path;
		int			suffix = 1;
		struct stat	st;

		while (stat(file_path.c_str(), &st) == 0)
		{
			size_t				dot_pos = part.filename.find_last_of('.');
			std::ostringstream	new_name;

			if (dot_pos != std::string::npos)
				new_name << upload_dir << "/" << part.filename.substr(0, dot_pos) << "_" << suffix << part.filename.substr(dot_pos);
			else
				new_name << upload_dir << "/" << part.filename << "_" << suffix;
			file_path = new_name.str();
			suffix++;
		}
		if (!writeFile(file_path, part.data))
			return (serve500());
		files_saved++;
		size_t		last_slash = file_path.find_last_of('/');
		std::string	saved_name = (last_slash != std::string::npos) ? file_path.substr(last_slash + 1) : file_path;
		saved_files.push_back(saved_name);
	}
	if (files_saved == 0)
	{
		Response	res;

		res.setStatus(400, "Bad Request");
		res.setHeader("Content-Type", "text/html");
		res.setBody("<html><body><h1>400 Bad Request</h1><p>No valid files found in upload.</p></body></html>");
		return (res);
	}

	Response	res;
	res.setStatus(201, "Created");
	res.setHeader("Content-Length", "0");

	std::string	location_path;
	if (location && !location->upload_store.empty())
	{
		std::string	store = location->upload_store;
		if (store.find(config.root) == 0)
			location_path = store.substr(config.root.length());
		else
			location_path = "/uploads";
	}
	else
		location_path = "/uploads";
	if (!saved_files.empty())
		res.setHeader("Location", location_path + "/" + saved_files[0]);
	return (res);
}

Response	Server::handleRawUpload(const Request& req, const LocationConfig* location)
{
	std::string	body = req.getBody();
	std::string	upload_dir = getUploadPath(location);

	mkdir(upload_dir.c_str(), 0755);

	std::string	filename = generateFilename();
	std::string	ct = req.getHeader("Content-Type");

	if (ct.find("text/") != std::string::npos)
		filename += ".txt";
	else if (ct.find("application/json") != std::string::npos)
		filename += ".json";
	else if (ct.find("image/") != std::string::npos)
	{
		if (ct.find("jpeg") != std::string::npos || ct.find("jpg") != std::string::npos)
			filename += ".jpg";
		else if (ct.find("png") != std::string::npos)
			filename += ".png";
		else if (ct.find("gif") != std::string::npos)
			filename += ".gif";
	}
	else
		filename += ".bin";

	std::string	file_path = upload_dir + "/" + filename;

	if (!writeFile(file_path, body))
		return (serve500());

	Response	res;
	res.setStatus(201, "Created");
	res.setHeader("Content-Length", "0");

	std::string	location_path;
	if (location && !location->upload_store.empty())
	{
		std::string	store = location->upload_store;
		if (store.find(config.root) == 0)
			location_path = store.substr(config.root.length());
		else
			location_path = "/uploads";
	}
	else
		location_path = "/uploads";
	res.setHeader("Location", location_path + "/" + filename);
	return (res);
}

bool	Server::deleteFile(const std::string& path)
{
	if (remove(path.c_str()) == 0)
		return (true);
	return (false);
}

Response	Server::handleDelete(const Request& req, const LocationConfig* location)
{
	std::string	file_path;

	if (location && !location->upload_store.empty())
	{
		std::string	uri = req.getPath();
		std::string	filename;

		if (uri.find(location->path) == 0)
		{
			filename = uri.substr(location->path.length());
			if (!filename.empty() && filename[0] == '/')
				filename = filename.substr(1);
		}
		else
			filename = uri;
		file_path = location->upload_store + "/" + filename;
	}
	else
		file_path = buildFilePath(req.getPath(), location);

	if (!fileExists(file_path))
		return (serve404());

	if (isDirectory(file_path))
		return (serve403());

	std::string	upload_dir = getUploadPath(location);
	std::string	root = config.root;

	bool		in_root = (file_path.find(root) == 0);
	bool		in_upload = (!upload_dir.empty() && file_path.find(upload_dir) == 0);
	
	if (!in_root && !in_upload)
		return (serve403());

	if (!deleteFile(file_path))
		return (serve500());

	Response	res;
	res.setStatus(204, "No Content");
	res.setHeader("Content-Length", "0");
	return (res);
}
