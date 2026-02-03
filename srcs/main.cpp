#include <iostream>
#include <csignal>
#include "ServerManager.hpp"
#include "Config.hpp"
#include <cstdlib>

// Global pointer for signal handling
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
	// Setup signal handler for Ctrl+C
	signal(SIGINT, signalHandler);
	
	// Check for command-line argument (config file is required)
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
	
	std::string config_file = argv[1];
	
	// Parse config file
	Config config;
	if (!config.parse(config_file))
	{
		std::cerr << "Failed to parse config file: " << config_file << std::endl;
		return 1;
	}
	
	// Print loaded configuration
	config.print();
	
	// Get servers from config
	const std::vector<ServerConfig>& servers = config.getServers();
	
	// Check if we have any servers
	if (servers.empty())
	{
		std::cerr << "No servers configured" << std::endl;
		return 1;
	}
	
	// Create server manager
	ServerManager manager;
	g_server_manager = &manager;
	
	// Initialize all servers
	if (!manager.initServers(servers))
	{
		std::cerr << "Failed to initialize servers" << std::endl;
		return 1;
	}
	
	// Run server manager (infinite loop with poll)
	manager.run();
	
	return 0;
}