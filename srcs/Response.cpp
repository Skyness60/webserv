#include "Response.hpp"
#include <sys/stat.h>

Response::Response(int fd, ClientRequest &request, Config &serv_conf, int index) 
    : _client_fd(fd), _config(serv_conf), _indexServ(index), _request(request) {
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
    : _client_fd(other._client_fd), _request(other._request), _config(other._config) {}

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
    std::string fullPath = "./www" + this->_request.getPath();

    // Check if the path is a directory
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        // Append the default index file if the path is a directory
        std::string indexFile = _config.getConfigValue(_indexServ, "index");
        fullPath += "/" + indexFile;
    }

    std::ifstream file(fullPath.c_str(), std::ios::binary);
	if (!file.is_open()) {
		// Serve error page if file not found
		std::string errorPage = _config.getConfigValue(_indexServ, "error_page");
		std::cout << errorPage << std::endl;
		std::string errorCode = errorPage.substr(0, errorPage.find(' ')); // Extract error code
		std::cout << errorCode << std::endl;
		std::string errorFile = errorPage.substr(errorPage.find(' ') + 1); // Extract error file
		std::cout << errorFile << std::endl;
	
		if (errorCode == "404") {
			fullPath = "./www/" + errorFile;

			// Validate that the error page is a file
			if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
				sendResponse(500, "Internal Server Error", "Error page path is a directory.");
				return;
			}

			file.open(fullPath.c_str(), std::ios::binary);
			if (!file.is_open()) {
				// Redirect to error page if it cannot be served
				std::ostringstream headers;
				headers << "HTTP/1.1 302 Found\r\n";
				headers << "Location: " << errorFile << "\r\n";
				headers << "\r\n";
				send(_client_fd, headers.str().c_str(), headers.str().size(), 0);
				return;
			}
		} else {
			sendResponse(404, "Not Found", "The requested file does not exist.");
			return;
		}
	}
	if (!file.is_open()) {
		// Serve error page if file not found
		std::string errorPage = _config.getConfigValue(_indexServ, "error_page");
		std::string errorCode = errorPage.substr(0, errorPage.find(' ')); // Extract error code
		std::string errorFile = errorPage.substr(errorPage.find(' ') + 1); // Extract error file

		if (errorCode == "404") {
			fullPath = "./www/" + errorFile;

			// Validate that the error page is a file
			if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
				sendResponse(500, "Internal Server Error", "Error page path is a directory.");
				return;
			}

			file.open(fullPath.c_str(), std::ios::binary);
			if (!file.is_open()) {
				// Redirect to error page if it cannot be served
				std::ostringstream headers;
				headers << "HTTP/1.1 302 Found\r\n";
				headers << "Location: " << errorFile << "\r\n";
				headers << "\r\n";
				send(_client_fd, headers.str().c_str(), headers.str().size(), 0);
				return;
			}
		} else {
			sendResponse(404, "Not Found", "The requested file does not exist.");
			return;
		}
	}

    // Read file content
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Determine the correct Content-Type
    std::string contentType = getContentType(fullPath);

    // Send response
    std::ostringstream headers;
    headers << "HTTP/1.1 200 OK\r\n";
    headers << "Content-Length: " << size << "\r\n";
    headers << "Content-Type: " << contentType << "\r\n";
    headers << "\r\n";

    send(_client_fd, headers.str().c_str(), headers.str().size(), 0);
    send(_client_fd, body.c_str(), body.size(), 0);
}

void Response::dealDelete() {
    std::string fullPath = "./www" + this->_request.getPath();

    if (access(fullPath.c_str(), F_OK) != 0) {
        sendResponse(404, "Not Found", "The requested file does not exist.");
        return;
    }

    if (remove(fullPath.c_str()) == 0) {
        sendResponse(200, "OK", "File deleted successfully.");
    } else {
        sendResponse(500, "Internal Server Error", "Failed to delete the file.");
    }
}

bool	Response::checkPost(std::string postUrl){
	std::vector<std::string> vector = this->_config.getLocationName(_indexServ);
	for (unsigned long i = 0; i < vector.size(); i++)
	if (!vector[i].find(postUrl))
	return 1;
	return 0;
}


static std::string getPostUrl(std::map <std::string, std::string> headers){
	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++){
		if (!it->first.compare("POST"))
			return (it->second);
	}
	return "";
}


void Response::dealPost(){
	std::string postUrl = getPostUrl(this->_request.getHeaders());
	if (!checkPost(postUrl)){
		std::string headers = "HTTP/1.1 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: ";
		std::ifstream in(("../www/error405.html"), std::ios::binary);
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		headers += toString(size) + "\r\n\r\n";
		std::string body ((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		send(this->_client_fd, headers.c_str(), headers.length(), 0);
		send(this->_client_fd, body.c_str(), body.length(), 0);
	}
	else {
		std::ofstream out ("playlist.txt");
		if (out.is_open()){
			if (this->_request.getHeaders()["Content-Type"] == "application/x-www-form-urlencoded"){
				for (std::map<std::string, std::string>::iterator it = this->_request.getFormData().begin(); it != this->_request.getFormData().end(); it++){
					out << it->first << " : " << it->second << std::endl;
				}
			}
			else	
				out << this->_request.getBody();
			out.close();
		}
		else {
			std::string headers = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: ";
			std::string body = "Failed to open file for writing.";
			headers += toString(body.length()) + "\r\n\r\n";
			send(this->_client_fd, headers.c_str(), headers.length(), 0);
			send(this->_client_fd, body.c_str(), body.length(), 0);
			return ;
		}
		std::string headers = "HTTP/1.1 201 OK\r\nContent-Type: text/html\r\nContent-Length: ";
		std::string body = "File created successfully.";
		headers += toString(body.length()) + "\r\n\r\n";
		send(this->_client_fd, headers.c_str(), headers.length(), 0);
		send(this->_client_fd, body.c_str(), body.length(), 0);
	}
}

void Response::sendResponse(int statusCode, const std::string &statusMessage, const std::string &body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Content-Type: text/plain\r\n";
    response << "\r\n";
    response << body;
    send(_client_fd, response.str().c_str(), response.str().size(), 0);
}

void Response::oriente() {
    for (size_t i = 0; i < 3; i++) {
        if (this->_func[i].first == this->_request.getMethod()) {
            return (this->*(_func[i].second))();
        }
    }
    sendResponse(405, "Method Not Allowed", "The requested method is not supported.");
}
