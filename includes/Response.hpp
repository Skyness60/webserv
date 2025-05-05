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
		std::pair<std::string, void (Response::*)()> _func[3];
		ClientRequest &_request;
		int _indexServ;
		bool isCGI(std::string path);
		void safeSend(int statusCode, const std::string &statusMessage, const std::string &body, const std::string &contentType = "text/html");
		std::string _requestMethod;
		std::string _requestPath;
		std::map<std::string, std::string> _requestHeaders;
		std::string _requestBody;

	public :
		Response(int fd, ClientRequest &request, Config &serv_conf, int index);
		~Response();
		Response(const Response &other);
		Response &operator=(const Response &other);
		void dealGet();
		void dealPost();
		void dealDelete();
		void oriente();
};
