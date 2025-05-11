#pragma once

#include "Includes.hpp"


class ServerManager {
	private:
		std::string _filename;
		Config _config;
		int _port;
		int createAndBindSocket(int serverIndex);
		struct sockaddr_in getServerAddress(int serverIndex);
		int setupEpollInstance(const std::vector<int> &server_fds);
		void eventLoop(int epoll_fd, const std::vector<int> &server_fds);
		void handleNewConnection(int server_fd, int epoll_fd);
		void handleClientRequest(int client_fd, int epoll_fd);
		void cleanup(int epoll_fd, const std::vector<int> &server_fds);
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
		int getServersCount();
		std::vector<std::string> getLocationName(int index);
		int getLocationCount(int serverIndex);
};