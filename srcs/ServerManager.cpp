#include "ServerManager.hpp"
#include "SocketManager.hpp"
#include "EpollManager.hpp"
#include "SignalHandler.hpp"
#include "DdosProtection.hpp"

// Constructeur qui initialise le gestionnaire de serveur avec un fichier de configuration
ServerManager::ServerManager(std::string filename) : _filename(filename), _config(filename) {
    loadConfig();
}

// Constructeur de copie
ServerManager::ServerManager(const ServerManager &copy) : _filename(copy._filename), _config(copy._filename) {
    loadConfig();
}

// Opérateur d'affectation
ServerManager &ServerManager::operator=(const ServerManager &copy) {
    if (this != &copy) {
        _filename = copy._filename;
        _config = copy._config;
        loadConfig();
    }
    return *this;
}

// Destructeur
ServerManager::~ServerManager() {
    // Implémentation du destructeur
}

// Charge la configuration à partir du fichier
void ServerManager::loadConfig() {
    // Cette fonction est actuellement désactivée (code commenté)
    // Elle pourrait lire et afficher le contenu du fichier de configuration
}

// Fonction principale pour démarrer le serveur
void ServerManager::startServer() {
    SocketManager socketManager;
    EpollManager epollManager;

    std::vector<int> server_fds;
    for (int i = 0; i < getServersCount(); ++i) {
        std::string listenValue = getConfigValue(i, "listen");
        int server_fd = socketManager.createAndBindSocket(i, listenValue);
        if (server_fd != -1) server_fds.push_back(server_fd);
    }

    int epoll_fd = epollManager.setupEpollInstance(server_fds);
    if (epoll_fd == -1) return;

    SignalHandler::setupSignalHandlers();

    epollManager.eventLoop(epoll_fd, server_fds,
        [this, epoll_fd](int server_fd) { handleNewConnection(server_fd, epoll_fd); },
        [this, epoll_fd](int client_fd) { handleClientRequest(client_fd, epoll_fd); }
    );

    cleanup(epoll_fd, server_fds);
}

// Gère une nouvelle connexion client
void ServerManager::handleNewConnection(int server_fd, int epoll_fd) {
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

    std::cout << "Nouveau client connecté: " << client_fd << std::endl;
}

// Gère une requête client
void ServerManager::handleClientRequest(int client_fd, int epoll_fd) {

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
        
        if (fullRequest.find("\r\n\r\n") != std::string::npos && fullRequest.find("Content-Length:") != std::string::npos) {
            size_t headerEnd = fullRequest.find("\r\n\r\n");
            std::string headers = fullRequest.substr(0, headerEnd);
            
            size_t contentLengthPos = headers.find("Content-Length:");
            if (contentLengthPos != std::string::npos) {
                size_t valueStart = headers.find(":", contentLengthPos) + 1;
                size_t valueEnd = headers.find("\r\n", valueStart);
                std::string contentLengthStr = headers.substr(valueStart, valueEnd - valueStart);
                contentLengthStr.erase(0, contentLengthStr.find_first_not_of(" \t"));
                contentLengthStr.erase(contentLengthStr.find_last_not_of(" \t") + 1);
                
                size_t contentLength = std::stoul(contentLengthStr);
                size_t bodyStart = headerEnd + 4; // +4 for \r\n\r\n
                
                if (fullRequest.length() >= bodyStart + contentLength) {
                    break;
                }
            }
        }
    } while (valread > 0);
    
    if (totalRead <= 0) {
        std::cout << "Client déconnecté: " << client_fd << std::endl;
        close(client_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
    }
    else {
        ClientRequest request;
        if (request.parse(fullRequest, &ddosProtector)) {          
            Response response(client_fd, request, _config, 0);
            response.oriente();
        }
        else {
            std::cerr << "Échec de l'analyse de la requête du client: " << client_fd << std::endl;
        }
    }
}


// Nettoie les ressources utilisées par le serveur
void ServerManager::cleanup(int epoll_fd, const std::vector<int> &server_fds) {
    close(epoll_fd);
    for (std::vector<int>::const_iterator it = server_fds.begin(); it != server_fds.end(); ++it) {
        close(*it);
    }
}

// Récupère une valeur de configuration pour un serveur donné
std::string ServerManager::getConfigValue(int server, std::string key) {
    return _config.getConfigValue(server, key);
}

// Récupère toutes les valeurs de configuration
std::vector<std::map<std::string, std::string> > ServerManager::getConfigValues() {
    return _config.getConfigValues();
}

// Récupère les valeurs des locations
std::vector<std::map<std::string, std::map<std::string, std::string> > > ServerManager::getLocationValues() {
    return _config.getLocationValues();
}

// Récupère une valeur spécifique pour une location
std::string ServerManager::getLocationValue(int server, std::string locationKey, std::string valueKey) {
    return _config.getLocationValue(server, locationKey, valueKey);
}

// Récupère le nombre de serveurs configurés
int ServerManager::getServersCount() {
    return _config.getConfigValues().size();
}

// Récupère les noms des locations pour un serveur donné
std::vector<std::string> ServerManager::getLocationName(int index) {
    return _config.getLocationName(index);
}

// Récupère le nombre de locations pour un serveur donné
int ServerManager::getLocationCount(int serverIndex) {
	return _config.getLocationCount(serverIndex);
}