#include <iostream>
#include <csignal>
#include "ServerManager.hpp"
#include "Config.hpp"
#include <cstdlib>

ServerManager* g_server_manager = NULL;

void	signalHandler(int signum)
{
	(void)signum;

	std::cout << "\nShutting down..." << std::endl;
	if (g_server_manager)
		g_server_manager->stop();
	exit(0);
}

int	main(int argc, char** argv)
{
	signal(SIGINT, signalHandler);
	signal(SIGPIPE, SIG_IGN);
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return (1);
	}

	std::string	config_file = argv[1];
	Config		config;
	if (!config.parse(config_file))
	{
		std::cerr << "Failed to parse config file: " << config_file << std::endl;
		return (1);
	}
	config.print();

	const std::vector<ServerConfig>&	servers = config.getServers();
	if (servers.empty())
	{
		std::cerr << "No servers configured" << std::endl;
		return (1);
	}

	ServerManager	manager;
	g_server_manager = &manager;	
	if (!manager.initServers(servers))
	{
		std::cerr << "Failed to initialize servers" << std::endl;
		return (1);
	}
	manager.run();
	return (0);
}
