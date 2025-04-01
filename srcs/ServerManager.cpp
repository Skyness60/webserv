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
    int server_fd;                 // Identifiant de notre "porte d'entrée" principale
    struct sockaddr_in address;    // Structure qui va contenir l'adresse de notre serveur
    int opt = 1;                   // Option pour configurer la porte
    int addrlen = sizeof(address); // Taille de la structure d'adresse

	/* 
     * Création de la porte d'entrée principale (socket)
     * AF_INET = utilise IPv4 (le système d'adressage standard d'Internet)
     * SOCK_STREAM = utilise TCP (communication fiable où les données arrivent dans l'ordre)
     */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
		throw std::runtime_error("Could not create socket");

	/*
     * Configuration de la porte pour qu'elle puisse être réutilisée rapidement
     * Utile pendant le développement quand on redémarre souvent le serveur
     */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("Could not set socket options");
	
    /*
     * Définition de l'adresse où notre porte va être installée
     * sin_family = AF_INET (utilise IPv4)
     * sin_addr.s_addr = INADDR_ANY (accepte les connexions de n'importe quelle adresse)
     * sin_port = 8080 (le numéro de port où les clients doivent se connecter)
     */

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);  // htons convertit le numéro au format réseau

    /*
     * Attache la porte à l'adresse définie
     * C'est comme installer officiellement notre porte à l'adresse choisie
     */

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1)
		throw std::runtime_error("Could not bind socket");

    /*
     * Met la porte en mode "écoute"
     * Le serveur attend que des clients se connectent
     * Le chiffre 10 est la taille de la file d'attente (combien de clients peuvent attendre)
     */

	if (listen(server_fd, 10) == -1)
		throw std::runtime_error("Could not listen on socket");
	
	std::cout << "Server listening on port 8080" << std::endl;

    /*
     * Boucle infinie qui attend et accepte les connexions entrantes
     * Pour chaque nouvelle connexion, une nouvelle "porte" (socket) est créée
     * Mais actuellement, le code ne fait rien avec ces connexions!
     */

	while (true)
	{
		int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		if (new_socket == -1)
			throw std::runtime_error("Could not accept connection");
		
		std::cout << "New connection" << std::endl;
        // Ici il faudrait ajouter du code pour:
        // 1. Lire les données envoyées par le client
        // 2. Traiter ces données
        // 3. Envoyer une réponse
        // 4. Fermer la connexion ou continuer à communiquer
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