#include "ServerManager.hpp"

ServerManager::ServerManager(std::string filename) : _filename(filename), _config(filename)
{
	loadConfig();
}

ServerManager::ServerManager(const ServerManager &copy) : _filename(copy._filename), _config(copy._filename)
{
	loadConfig();
}

ServerManager &ServerManager::operator=(const ServerManager &copy)
{
	if (this != &copy)
	{
		_filename = copy._filename;
		_config = copy._config;
		loadConfig();
	}
	return *this;
}
ServerManager::~ServerManager()
{
	// Destructor implementation
}
void ServerManager::loadConfig()
{
	// std::ifstream configFile(_filename.c_str());
	// if (!configFile.is_open())
	// 	throw std::runtime_error("Could not open config file");
	// std::string line;
	// std::cout << "Loading config file: " << _filename << std::endl;
	// while (std::getline(configFile, line))
	// 	std::cout << line << std::endl;
	// configFile.close();
}

/*
 * Fonction qui démarre un serveur TCP/IP simple
 * Elle crée une "porte d'entrée" (socket) qui écoute les connexions entrantes
 * sur le port 8080
 */

void ServerManager::startServer()
{
	std::vector<int> server_fds;
	std::vector<struct sockaddr_in> addresses;

	// Create a socket and bind it for each server configuration
	for (int i = 0; i < getServersCount(); ++i)
	{
		int server_fd;
		struct sockaddr_in address;
		int opt = 1;

		// Check if "listen" key exists in the configuration
		std::string listenValue;
		try
		{
			listenValue = getConfigValue(i, "listen");
		}
		catch (const std::exception &e)
		{
			std::cerr << "Error: Missing 'listen' configuration for server " << i << ". Skipping this server." << std::endl;
			continue;
		}

		// Validate the listen value (e.g., ensure it's a valid port number)
		int port = std::atoi(listenValue.c_str());
		if (port <= 0 || port > 65535)
		{
			std::cerr << "Error: Invalid 'listen' value for server " << i << ": " << listenValue << ". Skipping this server." << std::endl;
			continue;
		}

		server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd == -1)
		{
			std::cerr << "Error: Could not create socket for server " << i << ". Skipping this server." << std::endl;
			continue;
		}

		if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
		{
			std::cerr << "Error: Could not set socket options for server " << i << ". Skipping this server." << std::endl;
			close(server_fd);
			continue;
		}

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(static_cast<uint16_t>(port));

		if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
		{
			std::cerr << "Error: Could not bind socket for server " << i << " on port " << port << ". Skipping this server." << std::endl;
			close(server_fd);
			continue;
		}

		if (listen(server_fd, 10) == -1)
		{
			std::cerr << "Error: Could not listen on socket for server " << i << " on port " << port << ". Skipping this server." << std::endl;
			close(server_fd);
			continue;
		}

		std::cout << "Server " << i << " listening on port " << port << std::endl;

		server_fds.push_back(server_fd);
		addresses.push_back(address);
	}

	// Infinite loop to handle incoming connections for all servers
	while (true)
	{
		for (size_t i = 0; i < server_fds.size(); ++i)
		{
			int addrlen = sizeof(addresses[i]);
			int new_socket = accept(server_fds[i], (struct sockaddr *)&addresses[i], (socklen_t *)&addrlen);
			if (new_socket == -1)
				continue; // Non-blocking accept, skip if no connection

			std::cout << "New connection on server " << i << " (port " << getConfigValue(static_cast<int>(i), "listen") << ")" << std::endl;

			char buffer[1024] = {0};
			int valread = read(new_socket, buffer, sizeof(buffer));
			if (valread > 0)
			{
				std::string rawRequest(buffer, valread);
				std::cout << "Received request: " << buffer << std::endl;
				ClientRequest request;
				if (request.parse(rawRequest))
				{
					std::cout << "Parsed request:" << std::endl;
					request.printRequest();
					// Handle the request here
					for (size_t toto = 0; toto < getLocationName(i).size(); toto++) 
						std::cout << getLocationName(i)[toto] << std::endl;
					Response response(new_socket, request.getPath(), request.getMethod(), _config, request.getHeaders(), i);
					response.oriente();
				}
				else
					throw std::runtime_error("Failed to parse request");
			}
		}
	}
}

std::string ServerManager::getConfigValue(int server, std::string key)
{
	return _config.getConfigValue(server, key);
}

std::vector<std::map<std::string, std::string> > ServerManager::getConfigValues()
{
	return _config.getConfigValues();
}

std::vector<std::map<std::string, std::map<std::string, std::string> > > ServerManager::getLocationValues()
{
	return _config.getLocationValues();
}

std::string ServerManager::getLocationValue(int server, std::string locationKey, std::string valueKey)
{
	return _config.getLocationValue(server, locationKey, valueKey);
}

int	ServerManager::getServersCount()
{
	return _config.getConfigValues().size();
}

std::vector<std::string> ServerManager::getLocationName(int index){
	return _config.getLocationName(index);
}
