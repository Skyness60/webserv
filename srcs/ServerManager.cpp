#include "ServerManager.hpp"
#include <sys/epoll.h>
#include <fcntl.h>

volatile bool stopServer = false;

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

void signalHandler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		stopServer = true;
	}
}

/*
 * Fonction qui démarre un serveur TCP/IP simple
 * Elle crée une "porte d'entrée" (socket) qui écoute les connexions entrantes
 * sur le port 8080
 */


void ServerManager::startServer() {
    std::vector<int> server_fds;
    std::vector<struct sockaddr_in> addresses;

	signal (SIGINT, signalHandler);
	signal (SIGTERM, signalHandler);

    // Create and configure server sockets
    for (int i = 0; i < getServersCount(); ++i) {
        int server_fd = createAndBindSocket(i);
        if (server_fd != -1) {
            server_fds.push_back(server_fd);
            struct sockaddr_in address = getServerAddress(i);
            addresses.push_back(address);
        }
    }

    int epoll_fd = setupEpollInstance(server_fds);
    if (epoll_fd == -1) {
        return;
	}

	while (!stopServer) {
    	eventLoop(epoll_fd, server_fds);
	}

    cleanup(epoll_fd, server_fds);
}

int ServerManager::createAndBindSocket(int serverIndex) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    std::string listenValue;
    try {
        listenValue = getConfigValue(serverIndex, "listen");
    } catch (const std::exception &e) {
        std::cerr << "Error: Missing 'listen' configuration for server " << serverIndex << ". Skipping this server." << std::endl;
        return -1;
    }

    int port = std::atoi(listenValue.c_str());
    if (port <= 0 || port > 65535) {
        std::cerr << "Error: Invalid 'listen' value for server " << serverIndex << ": " << listenValue << ". Skipping this server." << std::endl;
        return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error: Could not create socket for server " << serverIndex << ". Skipping this server." << std::endl;
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        std::cerr << "Error: Could not set socket options for server " << serverIndex << ". Skipping this server." << std::endl;
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        std::cerr << "Error: Could not bind socket for server " << serverIndex << " on port " << port << ". Skipping this server." << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1) {
        std::cerr << "Error: Could not listen on socket for server " << serverIndex << " on port " << port << ". Skipping this server." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server " << serverIndex << " listening on port " << port << std::endl;
    return server_fd;
}

struct sockaddr_in ServerManager::getServerAddress(int serverIndex) {
    struct sockaddr_in address;
	std::string listenValue = getConfigValue(serverIndex, "listen");
	int port = std::atoi(listenValue.c_str());

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(static_cast<uint16_t>(port));

	std::cout << "Server " << serverIndex << " address: " << inet_ntoa(address.sin_addr) << ":" << port << std::endl;
	return address;
}

int ServerManager::setupEpollInstance(const std::vector<int> &server_fds) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    for (std::vector<int>::const_iterator it = server_fds.begin(); it != server_fds.end(); ++it) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = *it;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, *it, &event) == -1) {
            perror("epoll_ctl");
            close(epoll_fd);
            return -1;
        }
    }
    return epoll_fd;
}

void ServerManager::eventLoop(int epoll_fd, const std::vector<int> &server_fds) {
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (!stopServer) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
			if (errno == EINTR) {
				continue; // Interrupted by signal, retry
			}
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            int event_fd = events[i].data.fd;

            if (std::find(server_fds.begin(), server_fds.end(), event_fd) != server_fds.end()) {
                handleNewConnection(epoll_fd, event_fd);
            } else {
                handleClientRequest(epoll_fd, event_fd);
            }
        }
    }
}

void ServerManager::handleNewConnection(int epoll_fd, int server_fd) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);
    if (client_fd == -1) {
        perror("accept");
        return;
    }

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct epoll_event client_event;
    client_event.events = EPOLLIN | EPOLLET;
    client_event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
        perror("epoll_ctl");
        close(client_fd);
        return;
    }

    std::cout << "New client connected: " << client_fd << std::endl;
}

void ServerManager::handleClientRequest(int epoll_fd, int client_fd) {
    char buffer[1024] = {0};
    int valread = read(client_fd, buffer, sizeof(buffer));
    if (valread <= 0) {
        std::cout << "Client disconnected: " << client_fd << std::endl;
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    } else {
        std::string rawRequest(buffer, valread);
        ClientRequest request;
        if (request.parse(rawRequest)) {
            std::string requestedPath = request.getResourcePath();
            std::string method = request.getMethod();
            std::map<std::string, std::string> headers = request.getHeaders();
            int serverIndex = 0; // Assuming single server for now, adjust as needed

            // Use Response to handle the request
            Response response(client_fd, requestedPath, method, _config, headers, serverIndex);
            response.oriente();
        } else {
            std::cerr << "Failed to parse request from client: " << client_fd << std::endl;
        }
    }
}

void ServerManager::cleanup(int epoll_fd, const std::vector<int> &server_fds) {
    close(epoll_fd);
    for (std::vector<int>::const_iterator it = server_fds.begin(); it != server_fds.end(); ++it) {
        close(*it);
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
