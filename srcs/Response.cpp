#include "Response.hpp"
#include <sys/stat.h>

Response::Response(int fd, ClientRequest &request, Config &serv_conf, int index) 
	: _client_fd(fd), _config(serv_conf), _request(request), _indexServ(index) {
    this->_func[0] = std::make_pair("GET", &Response::dealGet);
    this->_func[1] = std::make_pair("POST", &Response::dealPost);
    this->_func[2] = std::make_pair("DELETE", &Response::dealDelete);
    this->_requestMethod = request.getMethod();
    this->_requestPath = request.getPath();
    this->_requestHeaders = request.getHeaders();
    this->_requestBody = request.getBody();
}

Response::~Response() {}

Response &Response::operator=(const Response &other) {
    if (this == &other)
        return *this;
        
    this->_client_fd = other._client_fd;
    this->_request = other._request;
    this->_config = other._config;
    this->_indexServ = other._indexServ;
    this->_requestMethod = other._requestMethod;
    this->_requestPath = other._requestPath;
    this->_requestHeaders = other._requestHeaders;
    this->_requestBody = other._requestBody;
    
    for (size_t i = 0; i < 3; i++) {
        this->_func[i] = other._func[i];
    }
    
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
    std::string root = this->_config.getLocationValue(_indexServ, "location " + this->_request.getPath(), "root");
	std::cout << "root :" << root << std::endl;
    if (root.empty()) {
        root = this->_config.getConfigValue(_indexServ, "root");
    }

    std::string fullPath = root + this->_request.getPath();
    std::cout << "fullPath: " << fullPath << std::endl;

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        std::string indexFile = _config.getLocationValue(_indexServ, "location " + this->_request.getPath(), "index");
        if (indexFile.empty()) {
            indexFile = _config.getConfigValue(_indexServ, "index");
        }
        fullPath += "/" + indexFile;
    }

    if (isCGI(this->_request.getPath())) {
        CGIManager cgi(_config, _indexServ, this->_request);
        cgi.executeCGI(_client_fd, this->_request.getMethod());
        return;
    }

    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::string errorPage = _config.getConfigValue(_indexServ, "error_page");
        if (!errorPage.empty()) {
            std::string errorCode = errorPage.substr(0, errorPage.find(' '));
            std::string errorFile = errorPage.substr(errorPage.find(' ') + 1);
            if (errorCode == "404") {
                fullPath = "./www/" + errorFile;
                if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
                    safeSend(500, "Internal Server Error", "Error page path is a directory.", "text/plain");
                    return;
                }
                file.open(fullPath.c_str(), std::ios::binary);
                if (!file.is_open()) {
                    safeSend(404, "Not Found", "The requested file does not exist.", "text/plain");
                    return;
                }
            } else {
                safeSend(404, "Not Found", "The requested file does not exist.", "text/plain");
                return;
            }
        } else {
            safeSend(404, "Not Found", "The requested file does not exist.", "text/plain");
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
        safeSend(404, "Not Found", "The requested file does not exist.", "text/plain");
        return;
    }

    if (remove(fullPath.c_str()) == 0) {
        safeSend(200, "OK", "File deleted successfully.", "text/plain");
    } else {
        safeSend(500, "Internal Server Error", "Failed to delete the file.", "text/plain");
    }
}

void Response::dealPost() {
	std::cout << "POST request received" << std::endl;
    std::string requestPath = this->_requestPath;
    std::string fullPath = this->_config.getLocationValue(_indexServ, "location " + requestPath, "root") + requestPath;
	if (fullPath.empty()) {
		fullPath = this->_config.getConfigValue(_indexServ, "root") + requestPath;
	}

    if (isCGI(requestPath)) {
        CGIManager cgi(_config, _indexServ, this->_request);
        cgi.executeCGI(_client_fd, this->_request.getMethod());
        return;
    }


    size_t lastSlashPos = fullPath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        std::string dirPath = fullPath.substr(0, lastSlashPos);
        std::string mkdirCmd = "mkdir -p " + dirPath;
        system(mkdirCmd.c_str());
    }

    std::ofstream out(fullPath.c_str(), std::ios::trunc);
    if (out.is_open()) {
        if (this->_requestHeaders.find("Content-Type") != this->_requestHeaders.end() &&
            this->_requestHeaders.at("Content-Type") == "application/x-www-form-urlencoded") {
            std::map<std::string, std::string> formData = this->_request.getFormData();
            for (std::map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); it++) {
                out << it->first << " : " << it->second << std::endl;
            }
        } else {
            std::cout << "Writing body data to file" << std::endl;
            out << this->_requestBody;
            out.flush();
        }
        out.close();
        safeSend(201, "Created", "File created successfully.", "text/plain");
    } else {
        std::string errorMsg = "Failed to open file for writing: " + fullPath;
        safeSend(500, "Internal Server Error", errorMsg, "text/plain");
    }
}

void Response::safeSend(int statusCode, const std::string &statusMessage, const std::string &body, const std::string &contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
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
    safeSend(405, "Method Not Allowed", "The requested method is not supported.", "text/plain");
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
