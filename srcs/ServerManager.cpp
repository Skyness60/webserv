#include "ServerManager.hpp"
#include <sys/epoll.h>
#include <fcntl.h>

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

void ServerManager::startServer() {
    std::vector<int> server_fds;
    std::vector<struct sockaddr_in> addresses;

    // Create a socket and bind it for each server configuration
    for (int i = 0; i < getServersCount(); ++i) {
        int server_fd;
        struct sockaddr_in address;
        int opt = 1;

        std::string listenValue;
        try {
            listenValue = getConfigValue(i, "listen");
        } catch (const std::exception &e) {
            std::cerr << "Error: Missing 'listen' configuration for server " << i << ". Skipping this server." << std::endl;
            continue;
        }

        int port = std::atoi(listenValue.c_str());
        if (port <= 0 || port > 65535) {
            std::cerr << "Error: Invalid 'listen' value for server " << i << ": " << listenValue << ". Skipping this server." << std::endl;
            continue;
        }

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            std::cerr << "Error: Could not create socket for server " << i << ". Skipping this server." << std::endl;
            continue;
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
            std::cerr << "Error: Could not set socket options for server " << i << ". Skipping this server." << std::endl;
            close(server_fd);
            continue;
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(static_cast<uint16_t>(port));

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
            std::cerr << "Error: Could not bind socket for server " << i << " on port " << port << ". Skipping this server." << std::endl;
            close(server_fd);
            continue;
        }

        if (listen(server_fd, 10) == -1) {
            std::cerr << "Error: Could not listen on socket for server " << i << " on port " << port << ". Skipping this server." << std::endl;
            close(server_fd);
            continue;
        }

        std::cout << "Server " << i << " listening on port " << port << std::endl;

        server_fds.push_back(server_fd);
        addresses.push_back(address);
    }

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    // Add server sockets to epoll
    for (size_t i = 0; i < server_fds.size(); ++i) {
        struct epoll_event event;
        event.events = EPOLLIN; // Ready to read
        event.data.fd = server_fds[i];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fds[i], &event) == -1) {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }
    }

    // Event loop
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_events; ++i) {
            int event_fd = events[i].data.fd;

            // Check if it's a server socket (new connection)
            if (std::find(server_fds.begin(), server_fds.end(), event_fd) != server_fds.end()) {
                struct sockaddr_in client_address;
                socklen_t client_len = sizeof(client_address);
                int client_fd = accept(event_fd, (struct sockaddr *)&client_address, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                // Set client socket to non-blocking
                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                // Add client socket to epoll
                struct epoll_event client_event;
                client_event.events = EPOLLIN | EPOLLET; // Edge-triggered
                client_event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue;
                }

                std::cout << "New client connected: " << client_fd << std::endl;
            } else {
                // Handle client request
                char buffer[1024] = {0};
                int valread = read(event_fd, buffer, sizeof(buffer));
                if (valread <= 0) {
                    // Client disconnected
                    std::cout << "Client disconnected: " << event_fd << std::endl;
                    close(event_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, NULL);
                } else {
                    // Process request
                    std::string rawRequest(buffer, valread);
                    ClientRequest request;
                    if (request.parse(rawRequest)) {
                        Response response(event_fd, request.getPath(), request.getMethod(), _config, request.getHeaders(), 0);
                        response.oriente();
                    } else {
                        std::cerr << "Failed to parse request from client: " << event_fd << std::endl;
                    }
                }
            }
        }
    }

    // Cleanup
    close(epoll_fd);
    for (size_t i = 0; i < server_fds.size(); ++i) {
        close(server_fds[i]);
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
