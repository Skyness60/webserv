#pragma once

#include "Includes.hpp"

class Config; // Forward declaration of Config class
#include "Response.hpp"
#include "Config.hpp" // Inclure la définition complète ici

class Response{
	private :
		int _client_fd;
		std::string _path;
		std::string _method;
		Config	&_config;
		std::pair<std::string, void (Response::*)()> _func[3];
	public :
		Response(int fd, std::string file, std::string cmd, Config &serv_conf);
		~Response();
		Response(const Response &other);
		Response &operator=(const Response &other);
		void dealGet();
		void dealPost();
		void dealDelete();
		void oriente();
		void bad_method();
};
