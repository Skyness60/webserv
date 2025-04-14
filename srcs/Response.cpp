# include "Response.hpp"

Response::Response(int fd, std::string file, std::string cmd, Config &serv_conf, std::map<std::string, std::string> headers, int index) : _client_fd(fd), _path(file), _method(cmd), _config(serv_conf), _headers(headers) {
	this->_func[0] = std::make_pair("GET", &Response::dealGet);
	this->_func[1] = std::make_pair("POST", &Response::dealPost);
	this->_func[2] = std::make_pair("DELETE", &Response::dealDelete);
	this->_indexServ = index;
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

Response::Response(const Response &other)
    : _client_fd(other._client_fd), _path(other._path), _method(other._method), _config(other._config) {

}


static std::string getExtension(std::string path){
	if (path[path.length() - 3] == 'h')
		return "Content-Type: text/html\r\n";
	else if (path[path.length() - 3] == 'j')
		return "Content-Type: text/jpeg\r\n";
	else if (path[path.length() - 3] == '.')
		return "Content-Type: text/css\r\n";
	return "Content-Type: application/octet-stream\r\n";
}

static std::string getPostUrl(std::map <std::string, std::string> headers){
	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
		if (!it->first.compare("POST"))
			return (it->second);
	}
	return "";
}

// Binary -> flag pour qu il n y ait pas de conversion texte lors de l open qui peut creer des problemes au niveau de la size des fins de ligne notamment et ne pas renvoyer la valeur exacte avec tellg

void Response::dealGet(){
	std::ifstream in(("../www/" + this->_path).c_str(), std::ios::binary);
	if (!in.is_open())
		in.open("../www/error404.html", std::ios::binary);
	in.seekg(0, std::ios::end);
	size_t size = in.tellg();
	std::string headers = "HTTP/1.1 404 Not Found\r\n";
	headers += getExtension(this->_path);
	headers += "Content-Length: " + toString(size) + "\r\n";
	headers += "\r\n";
	std::string body ((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	send(this->_client_fd, headers.c_str(), headers.length(), 0);
	send(this->_client_fd, body.c_str(), body.length(), 0);
}

bool	Response::checkPost(std::string postUrl){
	std::vector<std::string> vector = this->_config.getLocationName(_indexServ);
	for (int i = 0; i < vector.size(); i++)
		if (!vector[i].find(postUrl))
			return 1;
	return 0;
}

void Response::dealPost(){
	std::string postUrl = getPostUrl(this->_headers);

}

void Response::dealDelete(){
	
}

void Response::oriente(){
	for (size_t i = 0; i < 3; i++){
		if (this->_func[i].first.compare(this->_method)) {
			(this->*(_func[i].second))();
			return ;
		}
	}
	//return this->bad_method();
}
