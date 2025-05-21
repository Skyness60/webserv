#pragma once

#include "Includes.hpp" // Include the necessary headers
class Response;

class CGIManager {
	private:
		Config &_config; // Référence à la configuration du serveur
		int _client_fd; // Descripteur de fichier du client
		int _serverIndex; // Index du serveur

		ClientRequest &_request; // Référence à la requête du client
		std::string _extension; // Extension du fichier CGI
		std::string _path; // L'interpreteur CGI
		std::string _root; // Le répertoire racine du serveur
		std::string _locationName; // Nom de la location de l'endroit du CGI
		std::map<std::string, std::string> _env; // Variables d'environnement pour le CGI
		std::vector<int> &_server_fds; // Descripteurs de fichiers des serveurs
		int &_epoll_fd; // Descripteur de fichier de l'epoll
		void initEnv(std::map<std::string, std::string> &env); // Initialise les variables d'environnement pour le CGI
		Response *_response; // Réponse associée à la requête
	public:
		CGIManager(Config &config, int serverIndex, ClientRequest &request, int client_fd, std::string locationName, std::vector<int> &server_fds, int &epoll_fd);
		CGIManager(const CGIManager &copy);
		CGIManager &operator=(const CGIManager &copy);
		~CGIManager();

		void executeCGI(const std::string &method);
		char **createEnvArray();
		std::pair<std::string, std::string> parseCGIResponse(const std::string &cgiOutput);
		std::string getPath() const;
		std::string getExtension() const;
		std::string getRoot() const;
		std::string getLocationName() const;
};
