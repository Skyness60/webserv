#pragma once

#include "Includes.hpp"


class ServerManager {
	private:
		std::string _filename;
		std::vector<int> _server_fds;
		int _epoll_fd;
		Config _config;
		int _port;
		int createAndBindSocket(int serverIndex);
		struct sockaddr_in getServerAddress(int serverIndex);
		int setupEpollInstance(const std::vector<int> &server_fds);
		void eventLoop(int epoll_fd, const std::vector<int> &server_fds);
		void handleNewConnection(int server_fd, int epoll_fd);
		void handleClientRequest(int client_fd, int epoll_fd);
		void handleWriteReady(int client_fd);
		std::string extractHostname(const std::string& hostHeader);
		int findServerByHostAndPort(const std::string& hostname, int port);
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
		void cleanup(int _epoll_fd, const std::vector<int> &server_fds);
};