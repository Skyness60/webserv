#include "SocketManager.hpp"

int SocketManager::createAndBindSocket(int serverIndex, const std::string &listenValue) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    int port = std::atoi(listenValue.c_str());
    if (port <= 0 || port > 65535) {
        std::cerr << "Erreur : Valeur 'listen' invalide pour le serveur " << serverIndex << ": " << listenValue << ". Serveur ignoré." << std::endl;
        return -1;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Erreur : Impossible de créer le socket pour le serveur " << serverIndex << ". Serveur ignoré." << std::endl;
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        std::cerr << "Erreur : Impossible de configurer les options du socket pour le serveur " << serverIndex << ". Serveur ignoré." << std::endl;
        close(server_fd);
        return -1;
    }

    address = getServerAddress(listenValue);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        std::cerr << "Erreur : Impossible de lier le socket pour le serveur " << serverIndex << " au port " << port << ". Serveur ignoré." << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 512) == -1) {
        std::cerr << "Erreur : Impossible de mettre en écoute le socket pour le serveur " << serverIndex << " au port " << port << ". Serveur ignoré." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << FGRN("Serveur ") << serverIndex << FGRN(" en écoute sur le port ") << port << std::endl;
    return server_fd;
}

struct sockaddr_in SocketManager::getServerAddress(const std::string &listenValue) {
    struct sockaddr_in address;
    int port = std::atoi(listenValue.c_str());

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<uint16_t>(port));

    return address;
}
