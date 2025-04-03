# include "Response.hpp"

Response::Response(int fd, std::string file, std::string cmd, Config &serv_conf) : _client_fd(fd), _path(file), _method(cmd) {
	this->_config = serv_conf;
	this->_list[0] = "GET";
	this->_list[1] = "POST";
	this->_list[2] = "DELETE";
	this->f[0] = &Response::dealGet;
	this->f[1] = &Response::dealPost;
	this->f[2] = &Response::dealDelete;
}

Response::~Response() {}


Response &Response::operator=(const Response &other){
	if (this == &other)
		return *this;
	this->_client_fd = other._client_fd;
	this->_method = other._method;
	this->_path = other._path;
	this->_config = other._config;
	return *this;
}

Response::Response(const Response &other) {
	*this = other;
}

void Response::modifSentValue(std::string str){
	this->_sentValue += str;
}

void Response::dealGet(){

}

void Response::dealPost(){

}

void Response::dealDelete(){

}

void Response::bad_method(){
	
}

void Response::oriente(){
	for (int i = 0; i < 3; i++){
		if (_list[i] == this->_method)
			return (this->*f[i])();
	}
	return this->bad_method();
}

void Response::sendClient(int code, std::string mthd){

}
