# include "Response.hpp"

Response::Response(int fd, std::string file, std::string cmd, Config &serv_conf) : _client_fd(fd), _path(file), _method(cmd) {
	this->_config = serv_conf;
	this->_func[0] = std::make_pair("GET", &Response::dealGet);
	this->_func[1] = std::make_pair("POST", &Response::dealPost);
	this->_func[2] = std::make_pair("DELETE", &Response::dealDelete);
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
	for (size_t i = 0; i < 3; i++){
		if (this->_func[i].first.compare(this->_method)) {
			(this->*(_func[i].second))();
			return ;
		}
	}
	return this->bad_method();
}

void Response::sendClient(int code, std::string mthd){
	(void)code;
	(void)mthd;
}
