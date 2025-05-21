#include "ServerManager.hpp"
#include "SocketManager.hpp"
#include "EpollManager.hpp"
#include "SignalHandler.hpp"
#include "DdosProtection.hpp"
#include "Logger.hpp"  


ServerManager::ServerManager(std::string filename) : _filename(filename), _config(filename) {
    loadConfig();
}


ServerManager::ServerManager(const ServerManager &copy) : _filename(copy._filename), _config(copy._filename) {
    loadConfig();
}


ServerManager &ServerManager::operator=(const ServerManager &copy) {
    if (this != &copy) {
        _filename = copy._filename;
        _config = copy._config;
        loadConfig();
    }
    return *this;
}


ServerManager::~ServerManager() {
    
} 


void ServerManager::loadConfig() {
    
    
}


void ServerManager::startServer() {
    LOG_INFO(FGRN("startServer called"));
    SocketManager socketManager;
    EpollManager epollManager;

    int serverCount = getServersCount();
    if (serverCount < 0) {
        return;
    }
    this->_server_fds.reserve(static_cast<int>(serverCount));
    for (int i = 0; i < getServersCount(); ++i) {
        std::string listenValue = getConfigValue(i, "listen");
        int server_fd = socketManager.createAndBindSocket(i, listenValue);
        if (server_fd != -1) this->_server_fds.push_back(server_fd);
    }
    this->_epoll_fd = epollManager.setupEpollInstance(this->_server_fds);
    if (this->_epoll_fd == -1) return;
    SignalHandler::setupSignalHandlers();
    epollManager.eventLoop(this->_epoll_fd, this->_server_fds,
        [this](int server_fd) { handleNewConnection(server_fd, this->_epoll_fd); },
        [this](int client_fd) { handleClientRequest(client_fd, this->_epoll_fd); },
		[this](int client_fd) { handleWriteReady(client_fd); }
    );

    cleanup(this->_epoll_fd, this->_server_fds);
}


void ServerManager::handleNewConnection(int server_fd, int epoll_fd) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);
    if (client_fd == -1) {
        perror("accept");
		close(server_fd);
        return;
    }
    LOG_INFO(FBLU("New connection: client_fd = ") << client_fd << FBLU(" on server_fd = ") << server_fd);

    
    struct sockaddr_in server_address;
    socklen_t server_len = sizeof(server_address);
    if (getsockname(server_fd, (struct sockaddr *)&server_address, &server_len) == -1) {
        perror("getsockname");
        close(client_fd);
        return;
    }
    int incomingPort = ntohs(server_address.sin_port);

    
    for (int i = 0; i < getServersCount(); ++i) {
        std::string listenValue = getConfigValue(i, "listen");
		if (!listenValue.empty())
		{
			int configuredPort = std::stoi(listenValue);
        	if (configuredPort == incomingPort) {
            	_port = i;
           		break;
        	}
		}


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
}


void ServerManager::handleClientRequest(int client_fd, int epoll_fd) {
    LOG_INFO(FGRN("handleClientRequest: client_fd = ") << client_fd);
    const size_t buffer_size = 8192; 
    static DdosProtection ddosProtector(DDOS_DEFAULT_RATE_WINDOW, 
        DDOS_DEFAULT_MAX_REQUESTS, 
        DDOS_DEFAULT_BLOCK_DURATION);
    char buffer[buffer_size] = {0};
    std::string fullRequest;
    ssize_t totalRead = 0;
    ssize_t valread = 0;
    
    do {
        valread = read(client_fd, buffer, buffer_size - 1);
        if (valread <= 0) {
            break;
        }
        buffer[valread] = '\0';
        fullRequest.append(buffer, valread);
        totalRead += valread;
        
        size_t headerEnd = fullRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            size_t contentLengthPos = fullRequest.find("Content-Length:");
            if (contentLengthPos != std::string::npos && contentLengthPos < headerEnd) {
                size_t valueStart = fullRequest.find(":", contentLengthPos) + 1;
                size_t valueEnd = fullRequest.find("\r\n", valueStart);
                std::string contentLengthStr = fullRequest.substr(valueStart, valueEnd - valueStart);
                contentLengthStr.erase(0, contentLengthStr.find_first_not_of(" \t"));
                contentLengthStr.erase(contentLengthStr.find_last_not_of(" \t") + 1);
                
                size_t contentLength = std::stoul(contentLengthStr);
                size_t bodyStart = headerEnd + 4;
                
                if (fullRequest.length() >= bodyStart + contentLength) {
                    break;
                }
            } else {
                break;
            }
        }
    } while (valread > 0);
    
    if (totalRead <= 0) {
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        return;
    }
    
    ClientRequest preRequest;
    size_t maxBodySize = ddosProtector.getMaxBodySize();
    if (not _config.getConfigValue(_port, "max_body_size").empty())
    {
        maxBodySize = std::stoul(_config.getConfigValue(_port, "max_body_size"));
    }
    size_t headerEnd = fullRequest.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string requestLine = fullRequest.substr(0, fullRequest.find("\r\n"));
        std::string headers = fullRequest.substr(fullRequest.find("\r\n") + 2, headerEnd - (fullRequest.find("\r\n") + 2));
        
        std::istringstream tempStream(requestLine + "\r\n" + headers + "\r\n\r\n");
        if (preRequest.parseMethod(tempStream)) {
            preRequest.parseHeaders(tempStream);
            
            if (!preRequest.isHttpVersionSupported()) {
                Response response(client_fd, preRequest, _config, this->_port, this->_server_fds, epoll_fd);
                response.handleHttpVersionNotSupported(preRequest.getHttpVersion());
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                return;
            }
            
            if (preRequest.isUriTooLong()) {
                Response response(client_fd, preRequest, _config, this->_port, this->_server_fds, epoll_fd);
                response.handleUriTooLong(REQUEST_MAX_URI_LENGTH);
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                return;
            }

            if (!preRequest.isBodySizeValid()) {
                Response response(client_fd, preRequest, _config, this->_port, this->_server_fds, epoll_fd);
                response.handlePayloadTooLarge(maxBodySize);
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                return;
            }
        }
    }
    
    ClientRequest request;
    if (request.parse(fullRequest, &ddosProtector)) {
        std::map<std::string, std::string> headers = request.getHeaders();
        
        if (headers.find("Host") != headers.end()) {
            std::string hostname = extractHostname(headers["Host"]);
            int serverIndex = findServerByHostAndPort(hostname, std::stoi(getConfigValue(_port, "listen")));
            if (serverIndex != -1) {
                _port = serverIndex;
            }
        }
        
        Response response(client_fd, request, _config, this->_port, this->_server_fds, epoll_fd);
        response.oriente();
    } else {
        ClientRequest emptyRequest;
        Response response(client_fd, emptyRequest, _config, this->_port, this->_server_fds, epoll_fd);
        response.safeSend(400, "Bad Request", "Bad Request: The server couldn't parse the request.\n", "text/plain", true);
        
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    }
}

void ServerManager::handleWriteReady(int client_fd) {
    LOG_INFO(FGRN("handleWriteReady: client_fd=") << client_fd);
    LOG_INFO(FGRN("handleWriteReady sending HTTP code: 200"));
    const char *message = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\nContent-Type: text/plain\r\n\r\nHello, world!";
    ssize_t bytes_sent = send(client_fd, message, strlen(message), 0);
    if (bytes_sent <= 0) {
        perror("send");
    }
    close(client_fd);
}




void ServerManager::cleanup(int _epoll_fd, const std::vector<int> &server_fds) {
    if (_epoll_fd != -1) {
        close(_epoll_fd);
    }
    for (std::vector<int>::const_iterator it = server_fds.begin(); it != server_fds.end(); ++it) {
        close(*it);
    }
    const_cast<std::vector<int>&>(server_fds).clear();
}


std::string ServerManager::getConfigValue(int server, std::string key) {
    return _config.getConfigValue(server, key);
}


std::vector<std::map<std::string, std::string> > ServerManager::getConfigValues() {
    return _config.getConfigValues();
}


std::vector<std::map<std::string, std::map<std::string, std::string> > > ServerManager::getLocationValues() {
    return _config.getLocationValues();
}


std::string ServerManager::getLocationValue(int server, std::string locationKey, std::string valueKey) {
    return _config.getLocationValue(server, locationKey, valueKey);
}


int ServerManager::getServersCount() {
    return _config.getConfigValues().size();
}


std::vector<std::string> ServerManager::getLocationName(int index) {
    return _config.getLocationName(index);
}


int ServerManager::getLocationCount(int serverIndex) {
	return _config.getLocationCount(serverIndex);
}


std::string ServerManager::extractHostname(const std::string& hostHeader) {
    
    size_t colonPos = hostHeader.find(':');
    if (colonPos != std::string::npos) {
        return hostHeader.substr(0, colonPos);
    }
    return hostHeader;
}

int ServerManager::findServerByHostAndPort(const std::string& hostname, int port) {
    int defaultServerIndex = -1;
    
    for (int i = 0; i < getServersCount(); ++i) {
        std::string listenValue = getConfigValue(i, "listen");
        std::string serverName = getConfigValue(i, "server_name");
        
        if (!listenValue.empty()) {
            int configuredPort = std::stoi(listenValue);
            if (configuredPort == port) {
                if (serverName == hostname) {
                    return i;
                }
                
                if (defaultServerIndex == -1) {
                    defaultServerIndex = i;
                }
            }
        }
    }
    return defaultServerIndex;
}