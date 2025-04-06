#pragma once

#include "Includes.hpp"

class ServerManager {
	private:
		std::string _filename;
		Config _config;
	public:
		ServerManager(std::string filename);
		ServerManager(const ServerManager &copy);
		ServerManager &operator=(const ServerManager &copy);
		~ServerManager();
		void loadConfig();
		void startServer();
		std::string getConfigValue(int server, std::string key);
		std::vector<std::map<std::string, std::string> > getConfigValues();
		std::vector<std::map<std::string, std::map<std::string, std::string> > > getLocationValues();
		std::string getLocationValue(int server, std::string locationKey, std::string valueKey);
};