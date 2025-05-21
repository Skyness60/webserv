#pragma once

#include "Includes.hpp"
#include "Config.hpp" 
#include "Response.hpp"
#include "ClientRequest.hpp"

class Config; 
class ClientRequest; 
class CGIManager;


class Response{
	private :
		int _client_fd;
		std::vector<int> &_server_fds;
		int &_epoll_fd;
		Config	&_config;
		std::pair<std::string, void (Response::*)()> _func[4];
		ClientRequest &_request;
		int _indexServ;
		bool isCGI(std::string path);
		std::string _requestMethod;
		std::string _requestPath;
		std::map<std::string, std::string> _requestHeaders;
		std::string _requestBody;
		std::string generateAutoIndex(const std::string &directoryPath, const std::string &requestPath);
		std::string _sessionId;           
		std::vector<std::pair<std::string, std::string>> _responseHeaders;

	public :
		Response(int fd, ClientRequest &request, Config &serv_conf, int index, std::vector<int> &server_fds, int &epoll_fd);
		~Response();
		Response(const Response &other);
		Response &operator=(const Response &other);
		void dealGet();
		void dealPost();
		void dealDelete();
		void oriente();
		void handlePayloadTooLarge(size_t maxSize);
		void handleUriTooLong(size_t maxLength);
		void handleHttpVersionNotSupported(const std::string &version);
		void handleRedirect(const std::string &redirectUrl);
		void safeSend(int statusCode, const std::string &statusMessage, const std::string &body, const std::string &contentType = "text/html", bool closeConnection = false);
		void addCookie(const std::string &name, const std::string &value, const std::string &path = "/", const std::string &domain = "", int maxAge = 0);
		void handleNotFound();
		void cleanup(int _epoll_fd, const std::vector<int> &server_fds);
		void setSessionData(const std::string &key, const std::string &value);
		std::string getSessionData(const std::string &key) const;
};
