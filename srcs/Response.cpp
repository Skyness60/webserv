#include "Response.hpp"
#include <sys/stat.h>

Response::Response(int fd, ClientRequest &request, Config &serv_conf, int index) 
	: _client_fd(fd), _config(serv_conf), _request(request), _indexServ(index) {
    this->_func[0] = std::make_pair("GET", &Response::dealGet);
    this->_func[1] = std::make_pair("POST", &Response::dealPost);
    this->_func[2] = std::make_pair("DELETE", &Response::dealDelete);
    
    // Make deep copies of important data to prevent memory corruption
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
    std::string fullPath = "./www" + this->_request.getPath();
    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {        std::string indexFile = _config.getConfigValue(_indexServ, "index");
        fullPath += "/" + indexFile;
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
				sendResponse(500, "Internal Server Error", "Error page path is a directory.");
				return;
			}

			file.open(fullPath.c_str(), std::ios::binary);
			if (!file.is_open()) {
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

bool Response::checkPost(std::string postUrl){
	if (postUrl.find("/Playlist/playlist.txt") != std::string::npos) {
		return true;
	}

	std::vector<std::string> vector = this->_config.getLocationName(_indexServ);
	for (unsigned long i = 0; i < vector.size(); i++) {
		if (postUrl.find(vector[i]) == 0) {
			return true;
		}
	}
	return false;
}

void Response::dealPost(){
    std::string requestPath = this->_requestPath;
    std::cout << "POST Request received for path: " << requestPath << std::endl;
    std::cout << "Request method: " << this->_requestMethod << std::endl;
    
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = this->_requestHeaders.begin(); 
         it != this->_requestHeaders.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
    
    std::cout << "Body length: " << this->_requestBody.length() << std::endl;
    std::string fullPath = "./www" + requestPath;
    std::cout << "Will write to file: " << fullPath << std::endl;
    
    if (requestPath != "/Playlist/playlist.txt") {
        std::cout << "POST URL not allowed: " << requestPath << std::endl;
        sendResponse(405, "Method Not Allowed", "The requested method is not allowed for this resource.");
        return;
    }
    
    size_t lastSlashPos = fullPath.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        std::string dirPath = fullPath.substr(0, lastSlashPos);
        std::string mkdirCmd = "mkdir -p " + dirPath;
        std::cout << "Creating directory: " << dirPath << std::endl;
        system(mkdirCmd.c_str());
    }
    
    std::cout << "Content-Type: " << (this->_requestHeaders.find("Content-Type") != this->_requestHeaders.end() ?
                                    this->_requestHeaders.at("Content-Type") : "not set") << std::endl;
    if (this->_requestBody.length() < 1000) {
        std::cout << "Body content: " << this->_requestBody << std::endl;
    } else {
        std::cout << "Body content too large to print" << std::endl;
    }
    
    std::cout << "Attempting to open file: " << fullPath << std::endl;
    std::ofstream out(fullPath.c_str(), std::ios::trunc);
    if (out.is_open()){
        std::cout << "File opened successfully!" << std::endl;
        if (this->_requestHeaders.find("Content-Type") != this->_requestHeaders.end() &&
            this->_requestHeaders.at("Content-Type") == "application/x-www-form-urlencoded"){
            std::map<std::string, std::string> formData = this->_request.getFormData();
            for (std::map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); it++){
                out << it->first << " : " << it->second << std::endl;
            }
        }
        else if (this->_requestHeaders.find("Content-Type") != this->_requestHeaders.end() &&
                 this->_requestHeaders.at("Content-Type") == "application/json"){
            std::cout << "Writing JSON data to file" << std::endl;
            out << this->_requestBody;
            out.flush(); 
        }
        else {    
            std::cout << "Writing generic body data to file" << std::endl;
            out << this->_requestBody;
            out.flush(); 
        }
        out.close();
        
        // Debug file creation
        struct stat buffer;
        if (stat(fullPath.c_str(), &buffer) == 0) {
            std::cout << "File successfully created/written with size: " << buffer.st_size << " bytes" << std::endl;
        } else {
            std::cout << "File verification failed after writing!" << std::endl;
        }
        
        // Send success response
        std::string responseBody = "File created successfully.";
        std::ostringstream headers;
        headers << "HTTP/1.1 201 Created\r\n";
        headers << "Content-Type: text/plain\r\n";
        headers << "Content-Length: " << responseBody.length() << "\r\n";
        headers << "\r\n";
        headers << responseBody;
        
        send(this->_client_fd, headers.str().c_str(), headers.str().size(), 0);
    }
    else {
        std::string errorMsg = "Failed to open file for writing: " + fullPath;
        sendResponse(500, "Internal Server Error", errorMsg);
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
