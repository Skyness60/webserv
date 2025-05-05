#include "Response.hpp"
#include <sys/stat.h>

Response::Response(int fd, ClientRequest &request, Config &serv_conf, int index) 
	: _client_fd(fd), _config(serv_conf), _request(request), _indexServ(index) {
    this->_func[0] = std::make_pair("GET", &Response::dealGet);
    this->_func[1] = std::make_pair("POST", &Response::dealPost);
    this->_func[2] = std::make_pair("DELETE", &Response::dealDelete);
}

Response::~Response() {}

Response &Response::operator=(const Response &other) {
    if (this == &other)
        return *this;
    this->_client_fd = other._client_fd;
    this->_request = other._request;
    this->_config = other._config;
    return *this;
}

Response::Response(const Response &other)
    : _client_fd(other._client_fd), _config(other._config), _request(other._request) {}

static std::string getContentType(const std::string &path) {
    if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return "text/css";
    if (path.find(".js") != std::string::npos)
        return "application/javascript";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    if (path.find(".png") != std::string::npos)
        return "image/png";
    return "application/octet-stream";
}

void Response::dealGet() {
    std::string fullPath = this->_config.getConfigValue(_indexServ, "root") + this->_request.getPath();

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        std::string indexFile = _config.getConfigValue(_indexServ, "index");
        fullPath += "/" + indexFile;
    }
	// Ici c'est la ou j'appelerais la fonction CGI (c'est pas encore fait et c'est pas une ia)
	if (isCGI(this->_request.getPath())) {
		CGIManager cgi(_config, _indexServ, this->_request);
		std::cout << "CGI path: " << cgi.getPath() << std::endl;
		std::cout << "CGI extension: " << cgi.getExtension() << std::endl;
		std::cout << "CGI root: " << cgi.getRoot() << std::endl;
		cgi.executeCGI(_client_fd, this->_request.getMethod());
		return;
	}

    std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		std::string errorPage = _config.getConfigValue(_indexServ, "error_page");
		std::cout << errorPage << std::endl;
		std::string errorCode = errorPage.substr(0, errorPage.find(' ')); // Extract error code
		std::cout << errorCode << std::endl;
		std::string errorFile = errorPage.substr(errorPage.find(' ') + 1); // Extract error file
		std::cout << errorFile << std::endl;
	
		if (errorCode == "404") {
			fullPath = "./www/" + errorFile;
			if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
				safeSend(500, "Internal Server Error", "Error page path is a directory.");
				return;
			}

			file.open(fullPath.c_str(), std::ios::binary);
			if (!file.is_open()) {
				safeSend(302, "Found", errorFile.c_str(), "text/html");
				return;
			}
		} else {
			safeSend(404, "Not Found", "The requested file does not exist.");
			return;
		}
	}
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string contentType = getContentType(fullPath);
	safeSend(200, "OK", body, contentType);
}

void Response::dealDelete() {
    std::string fullPath = "./www" + this->_request.getPath();

    if (access(fullPath.c_str(), F_OK) != 0) {
        safeSend(404, "Not Found", "The requested file does not exist.");
        return;
    }

    if (remove(fullPath.c_str()) == 0) {
        safeSend(200, "OK", "File deleted successfully.");
    } else {
        safeSend(500, "Internal Server Error", "Failed to delete the file.");
    }
}

void Response::dealPost(){
	std::string postUrl = _request.getPath();

	std::string fileName;
	for (size_t i = 0; postUrl[i] != '\0'; i++){
		char c = postUrl[i];
		if (isalnum(c) || c == '_' || c == '-'){
			fileName += c;
		}
		else {
			fileName += '_';
		}
	}
	std::string fullPath = "./www/" + fileName + ".txt";
	std::ofstream out (fullPath.c_str());
	if (out.is_open()){
		if (this->_request.getHeaders()["Content-Type"] == "application/x-www-form-urlencoded"){
			std::map<std::string, std::string> formData = this->_request.getFormData();
			for (std::map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); it++){
				out << it->first << " : " << it->second << std::endl;
			}
		}
		else
			out << this->_request.getBody();
		out.close();
	}
	else {
		std::string body = "<h>Failed to open file for writing.</h>";
		safeSend(500, "Internal Server Error", body, "text/html");
		return ;
	}
	std::string body = "<h>File created successfully.</h>";
	safeSend(201, "OK", body, "text/html");
}

void Response::safeSend(int statusCode, const std::string &statusMessage, const std::string &body, const std::string &contentType) {
	std::ostringstream response;
	response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	response << "Content-Length: " << body.size() << "\r\n";
	response << "Content-Type:" << contentType << "\r\n";
	response << "\r\n";
	response << body;
	std::string responseStr = response.str();
	ssize_t total = responseStr.size();
	ssize_t sent = 0;
	const char *dest = responseStr.c_str();
	while (sent < total) {
		ssize_t bytes = send(_client_fd, dest + sent, total - sent, MSG_NOSIGNAL);
		if (bytes == -1) {
			close(_client_fd);
			return;
		}
		sent += bytes;
	}
	close(_client_fd);
}

void Response::oriente() {
    for (size_t i = 0; i < 3; i++) {
        if (this->_func[i].first == this->_request.getMethod()) {
            return (this->*(_func[i].second))();
        }
    }
	std::ifstream file("./www/405.html");
	if (!file.is_open()) {
		safeSend(500, "Internal Server Error", "<h>Error page path is a directory.</h>", "text/html");
		return;
	}
	std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	safeSend(405, "Method Not Allowed", body.c_str(), "text/html");
}

bool Response::isCGI(std::string path) {\
	for (size_t i = 0; i < this->_config.getLocationName(_indexServ).size(); i++) {
		std::string location = this->_config.getLocationName(_indexServ)[i];
		location = location.substr(9);
		if (path.find(location) != std::string::npos) {
			std::cout << "REQUETE : " << this->_request.getPath() << std::endl;
			if (path.find(".py") != std::string::npos) {
				return true;
			}
			if (path.find(".pl") != std::string::npos) {
				return true;
			}
			if (path.find(".php") != std::string::npos) {
				return true;
			}
			if (path.find(".rb") != std::string::npos) {
				return true;
			}
			if (path.find(".cgi") != std::string::npos) {
				return true;
			}
			if (path.find(".sh") != std::string::npos) {
				return true;
			}
			if (path.find(".js") != std::string::npos) {
				return true;
			}
			if (path.find(".jsp") != std::string::npos) {
				return true;
			}
			if (path.find(".asp") != std::string::npos) {
				return true;
			}
		}
	}
	return false;
}
