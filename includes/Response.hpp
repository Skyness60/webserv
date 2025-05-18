#pragma once

#include "Includes.hpp"
#include "Config.hpp" // Inclure la définition complète ici
#include "Response.hpp"
#include "ClientRequest.hpp"

class Config; // Forward declaration of Config class
class ClientRequest; 
class CGIManager;


class Response{
	private :
		int _client_fd;
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
		void handleNotFound();
		std::string _sessionId;           // current session ID
		std::vector<std::pair<std::string, std::string>> _responseHeaders;

	public :
		Response(int fd, ClientRequest &request, Config &serv_conf, int index);
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
};
