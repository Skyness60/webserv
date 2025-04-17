#pragma once

#include "Includes.hpp"

class CGIManager {
	private:
		Config &_config; // Référence à la configuration du serveur
		int _serverIndex; // Index du serveur
		std::string _extension; // Extension du fichier CGI
		std::string _path; // L'interpreteur CGI
		std::string _root; // Le répertoire racine du serveur
		std::string _locationName; // Nom de la location de l'endroit du CGI
	public:
		CGIManager(Config &config, int serverIndex);
		CGIManager(const CGIManager &copy);
		CGIManager &operator=(const CGIManager &copy);
		~CGIManager();

		void executeCGI(int client_fd, const std::string &path, const std::string &method, const std::string &queryString);
		std::string getCGIPath(const std::string &path);
		std::string getCGIContentType(const std::string &path);
		std::string getCGIHeaders(int statusCode, const std::string &contentType, int contentLength);
};