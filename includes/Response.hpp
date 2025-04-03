#pragma once

#include "Includes.hpp"
#include "Config.hpp"
#include "Macros.hpp"

class Config;

class Response{
	private :
		int _client_fd;
		std::string _path;
		std::string _method;
		Config	_config;
		std::string _sentValue;
		std::string _list[3];
		void (Response::*f[3])();
	public :
		Response(int fd, std::string file, std::string cmd, Config &serv_conf);
		~Response();
		Response(const Response &other);
		Response &operator=(const Response &other);
		void modifSentValue(std::string str);
		void dealGet();
		void dealPost();
		void dealDelete();
		void oriente();
		void sendClient(int code, std::string mthd);
		void bad_method();
};
