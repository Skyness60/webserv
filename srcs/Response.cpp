#include "Response.hpp"
#include "SessionManager.hpp"
#include "Logger.hpp"
#include <sys/stat.h>
#include <dirent.h>

Response::Response(int fd, ClientRequest &request, Config &serv_conf, int index, std::vector<int> &server_fds, int &epoll_fd)
    : _client_fd(fd),
      _server_fds(server_fds),
      _epoll_fd(epoll_fd),
      _config(serv_conf),
      _request(request),
      _indexServ(index)
{
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
    this->_server_fds = other._server_fds;
    this->_client_fd = other._client_fd;
    this->_epoll_fd = other._epoll_fd;
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
    : _client_fd(other._client_fd),
      _server_fds(other._server_fds),
        _epoll_fd(other._epoll_fd),
      _config(other._config),
      _request(other._request),
      _indexServ(other._indexServ),
      _requestMethod(other._requestMethod),
      _requestPath(other._requestPath),
      _requestHeaders(other._requestHeaders),
      _requestBody(other._requestBody),
      _sessionId(other._sessionId),
      _responseHeaders(other._responseHeaders)
{
    for (size_t i = 0; i < 3; i++) {
        this->_func[i] = other._func[i];
    }
}

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

static SessionManager sessionManager(3600); 


static std::string parseCookieHeader(const std::string &hdr, const std::string &name) {
    std::istringstream iss(hdr);
    std::string token;
    while (std::getline(iss, token, ';')) {
        
        size_t start = token.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = token.find_last_not_of(" \t");
        std::string pair = token.substr(start, end - start + 1);

        size_t eq = pair.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = pair.substr(0, eq);
        std::string value = pair.substr(eq + 1);

        if (key == name)
            return value;
    }
    return "";
}

void Response::dealGet() {
	LOG_INFO(FGRN("dealGet: client_fd = " ) << _client_fd);
	std::string alias = this->_config.getLocationValue(_indexServ, this->_request.getPath(), "alias");
    std::string root = this->_config.getLocationValue(_indexServ, this->_request.getPath(), "root");
    if (root.empty()) {
        root = this->_config.getConfigValue(_indexServ, "root");
    }

    std::string fullPath = root + this->_request.getPath();
	if (!alias.empty()) {
		fullPath = alias;
	}

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
        std::string indexFile = _config.getLocationValue(_indexServ, this->_request.getPath(), "index");
        if (indexFile.empty() && this->_request.getPath() == "/") {
            indexFile = _config.getConfigValue(_indexServ, "index");
        } 
        
        if (!indexFile.empty()) {
            fullPath += "/" + indexFile;
        } else {
            std::string autoIndex = _config.getLocationValue(_indexServ, this->_request.getPath(), "autoindex");
            if (autoIndex == "on") {
                std::string autoIndexPage = generateAutoIndex(fullPath, this->_request.getPath());
                safeSend(200, "OK", autoIndexPage, "text/html");
                return;
            } else {
                safeSend(403, "Forbidden", "Directory listing not allowed.", "text/plain");
                return;
            }
        }
    }

    if (isCGI(this->_request.getPath())) {
        std::string cgiPath;
        size_t lastSlash = this->_request.getPath().find_last_of('/');
        std::string cgiRequestPath = this->_request.getPath();
        if (lastSlash != std::string::npos) {
            cgiRequestPath = this->_request.getPath().substr(0, lastSlash + 1);
        }
        std::string cgiRoot = this->_config.getLocationValue(_indexServ, cgiRequestPath, "cgi_root");
        std::string alias = this->_config.getLocationValue(_indexServ, cgiRequestPath, "alias");
        if (!alias.empty()) {
            cgiPath = alias;
        } else if (!cgiRoot.empty()) {
            cgiPath = cgiRoot;
        } else {
            cgiPath = fullPath;
        }
        struct stat cgiStat;
        if (stat(cgiPath.c_str(), &cgiStat) != 0) {
            handleNotFound();
            return;
        }
        CGIManager cgi(_config, _indexServ, this->_request, _client_fd, this->_request.getPath(), this->_server_fds, _epoll_fd);
        cgi.executeCGI(this->_request.getMethod());
        close(_client_fd);
        return;
    }

    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        handleNotFound();
        return;
    }

    std::string body((std::istreambuf_iterator<char>(file)), {});
    std::string contentType = getContentType(fullPath);

    
    {
        std::string msg = sessionManager.getSessionData(_sessionId, "message");
        if (!msg.empty()) {
            std::string snippet = "<div class=\"session-message\">" + msg + "</div>";
            const std::string placeholder = "<!-- SESSION_MESSAGE -->";
            size_t pos = body.find(placeholder);
            if (pos != std::string::npos) {
                body.replace(pos, placeholder.length(), snippet);
            } else {
                size_t bodyClose = body.rfind("</body>");
                if (bodyClose != std::string::npos) {
                    body.insert(bodyClose, snippet);
                }
            }
        }
    }
    
    {
        std::string snippet = "<div class=\"session-id\">Session ID: " + _sessionId + "</div>";
        const std::string placeholder = "<!-- SESSION_ID -->";
        size_t pos = body.find(placeholder);
        if (pos != std::string::npos) {
            body.replace(pos, placeholder.length(), snippet);
        } else {
            
            size_t headerOpen = body.find("<header");
            if (headerOpen != std::string::npos) {
                size_t tagEnd = body.find('>', headerOpen);
                if (tagEnd != std::string::npos) {
                    body.insert(tagEnd + 1, snippet);
                }
            }
        }
    }

    safeSend(200, "OK", body, contentType);
}

std::string Response::generateAutoIndex(const std::string &directoryPath, const std::string &requestPath) {
    DIR *dir = opendir(directoryPath.c_str());
    if (!dir) {
        return "<html><body><h1>500 Internal Server Error</h1><p>Failed to open directory.</p></body></html>";
    }

    std::ostringstream html;
    html << "<html><head><title>Index of " << requestPath << "</title></head><body>";
    html << "<h1>Index of " << requestPath << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        html << "<li><a href=\"" << requestPath;
        if (requestPath.back() != '/') html << "/";
        html << name << "\">" << name << "</a></li>";
    }

    html << "</ul></body></html>";
    closedir(dir);
    return html.str();
}

void Response::dealDelete() {
    LOG_INFO(FGRN("dealDelete: client_fd = " ) << _client_fd);
    std::string requestPath = this->_requestPath;
    std::string rootPath = "";
    rootPath = this->_config.getLocationValue(_indexServ, requestPath, "root");
    
    if (rootPath.empty()) {
        size_t lastSlash = requestPath.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0) {
            std::string parentPath = requestPath.substr(0, lastSlash + 1);
            rootPath = this->_config.getLocationValue(_indexServ, parentPath, "root");
            
            if (rootPath.empty() && lastSlash > 0) {
                parentPath = requestPath.substr(0, lastSlash);
                rootPath = this->_config.getLocationValue(_indexServ, parentPath, "root");
            }
        }
    }
    
    if (rootPath.empty()) {
        rootPath = this->_config.getConfigValue(_indexServ, "root");
    }
    
    std::string fileName = requestPath;
    size_t lastSlash = requestPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        fileName = requestPath.substr(lastSlash + 1);
    }
    
    std::string fullPath = rootPath + "/" + fileName;

    if (access(fullPath.c_str(), F_OK) != 0) {
        safeSend(404, "Not Found", "The requested file does not exist.\n", "text/plain");
        return;
    }

    if (access(fullPath.c_str(), W_OK) != 0) {
        safeSend(403, "Forbidden", "You don't have permission to delete this file.\n", "text/plain");
        return;
    }

    if (remove(fullPath.c_str()) == 0) {
        safeSend(200, "OK", "File deleted successfully.\n", "text/plain");
    } else {
        safeSend(500, "Internal Server Error", "Failed to delete the file.\n", "text/plain");
    }
}

void Response::dealPost() {
    LOG_INFO(FGRN("dealPost: client_fd= ") << _client_fd);
    std::string requestPath = this->_requestPath;
    std::string rootPath = "";

    rootPath = this->_config.getLocationValue(_indexServ, requestPath, "root");
    
    if (rootPath.empty()) {
        size_t lastSlash = requestPath.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0) {
            
            std::string parentPath = requestPath.substr(0, lastSlash + 1);
            rootPath = this->_config.getLocationValue(_indexServ, parentPath, "root");
            
            if (rootPath.empty() && lastSlash > 0) {
                parentPath = requestPath.substr(0, lastSlash);
                rootPath = this->_config.getLocationValue(_indexServ, parentPath, "root");
            }
        }
    }
    
    if (rootPath.empty()) {
        rootPath = this->_config.getConfigValue(_indexServ, "root");
    }
    
    std::string fileName = requestPath;
    size_t lastSlash = requestPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        fileName = requestPath.substr(lastSlash + 1);
    }
    
    if (fileName.empty()) {
        fileName = "index.html";
    }

    std::string fullPath = rootPath + "/" + fileName;
    
    if (isCGI(requestPath)) {
        CGIManager cgi(_config, _indexServ, this->_request, _client_fd, this->_request.getPath(), this->_server_fds, _epoll_fd);
        cgi.executeCGI(this->_request.getMethod());
        close(_client_fd);
        return;
    }

    std::string dirPath = rootPath;
    std::string mkdirCmd = "mkdir -p \"" + dirPath + "\"";
    system(mkdirCmd.c_str());
    
    std::ofstream out(fullPath.c_str(), std::ios::trunc);
    if (out.is_open()) {
        if (this->_requestHeaders.find("Content-Type") != this->_requestHeaders.end() &&
            this->_requestHeaders.at("Content-Type") == "application/x-www-form-urlencoded") {
            std::map<std::string, std::string> formData = this->_request.getFormData();
            for (std::map<std::string, std::string>::iterator it = formData.begin(); it != formData.end(); it++) {
                out << it->first << " : " << it->second << std::endl;
            }
        } else {
            out << this->_requestBody;
            out.flush();
        }
        out.close();
        safeSend(201, "Created", "File created successfully.\n", "text/plain");
    } else {
        safeSend(500, "Internal Server Error", "Failed to open file for writing.", "text/plain");
    }
}

void Response::safeSend(int statusCode,
                        const std::string &statusMessage,
                        const std::string &body,
                        const std::string &contentType,
                        bool closeConnection)
{
    LOG_INFO(FGRN("Sending HTTP code: ") << statusCode);
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << (closeConnection || statusCode == 413
                 ? "Connection: close\r\n"
                 : "Connection: keep-alive\r\n");
    if (statusCode == 405)
        response << "Allow: GET, POST, DELETE\r\n";
    
    for (size_t i = 0; i < _responseHeaders.size(); ++i) {
        response << _responseHeaders[i].first
                 << ": " << _responseHeaders[i].second << "\r\n";
    }
    response << "\r\n" << body;

    std::string respStr = response.str();
    if (send(_client_fd, respStr.c_str(), respStr.size(), MSG_NOSIGNAL) <= 0) {
		return ;
    }
    _responseHeaders.clear();
    close(_client_fd);
}

void Response::handlePayloadTooLarge(size_t maxSize) {
    std::ostringstream message;
    message << "The request body exceeds the maximum allowed size of " 
            << (maxSize / 1024 / 1024) << " MB.";
    
    safeSend(413, "Payload Too Large", message.str(), "text/plain", true);
}

void Response::handleUriTooLong(size_t maxLength) {
    std::ostringstream message;
    message << "The requested URI exceeds the maximum allowed length of " 
            << maxLength << " characters.\n";
    
    safeSend(414, "URI Too Long", message.str(), "text/plain", true);
}

void Response::oriente() {
    
    sessionManager.cleanupExpiredSessions();

    {
        std::string cookieHdr = _requestHeaders.count("Cookie")
                                ? _requestHeaders["Cookie"]
                                : "";
        std::string sid = parseCookieHeader(cookieHdr, "SESSIONID");
        if (sid.empty()) {
            
            sid = sessionManager.createSession();
        } else {
            
            if (!sessionManager.sessionExists(sid)) {
                sessionManager.addSession(sid);
            }
        }
        _sessionId = sid;
        
        addCookie("SESSIONID", sid, "/", "", 3600);
    }

    
    std::string requestPath = this->_request.getPath();
    std::string redirectDirective = this->_config.getLocationValue(_indexServ, requestPath, "return");
    
    if (!redirectDirective.empty()) {
        size_t spacePos = redirectDirective.find(' ');
        if (spacePos != std::string::npos) {
            std::string statusCodeStr = redirectDirective.substr(0, spacePos);
            std::string redirectUrl = redirectDirective.substr(spacePos + 1);
            
            int statusCode = std::atoi(statusCodeStr.c_str());
            if (statusCode == 301) {
                handleRedirect(redirectUrl);
                return;
            }
        }
    }
    
    bool isMethodSupported = (this->_request.getMethod() == "GET" || 
                              this->_request.getMethod() == "POST" || 
                              this->_request.getMethod() == "DELETE");
    
    if (!isMethodSupported) {
        safeSend(405, "Method Not Allowed", "The requested method is not supported for this location.\n", "text/plain");
        return;
    }
    
    std::string allowedMethods = this->_config.getLocationValue(_indexServ, requestPath, "methods");
    
    if (allowedMethods.empty()) {
        allowedMethods = "GET POST DELETE";
    }
    
    if (allowedMethods.find(this->_request.getMethod()) == std::string::npos) {
        safeSend(405, "Method Not Allowed", "The requested method is not supported for this location.\n", "text/plain");
        return;
    }

    for (size_t i = 0; i < 3; i++) {
        if (this->_func[i].first == this->_request.getMethod()) {
            return (this->*(_func[i].second))();
        }
    }
    safeSend(500, "Internal Server Error", "An unexpected error occurred processing your request.\n", "text/plain");
}

bool Response::isCGI(std::string path) {
    
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot + 1);

    
    for (std::string loc : _config.getLocationName(_indexServ)) {
        std::string extList = _config.getLocationValue(_indexServ, loc, "cgi_extension");
        if (extList.empty()) continue;
        std::istringstream iss(extList);
        std::string token;
        while (iss >> token) {
            if (token == ext || token == "." + ext)
                return true;
        }
    }
    return false;
}

void Response::handleNotFound() {
    std::string errorPage = _config.getConfigValue(_indexServ, "error_page");
    if (!errorPage.empty()) {
        size_t spacePos = errorPage.find(' ');
        if (spacePos != std::string::npos) {
            std::string errorCode = errorPage.substr(0, spacePos);
            std::string errorFile = errorPage.substr(spacePos + 1);
            
            if (errorCode == "404") {
                std::string fullPath = _config.getConfigValue(_indexServ, "root") + errorFile;
                struct stat pathStat;
                if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
                    safeSend(500, "Internal Server Error", "Error page path is a directory.\n", "text/plain");
                    return;
                }
                
                std::ifstream file(fullPath.c_str(), std::ios::binary);
                if (file.is_open()) {
                    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    std::string contentType = getContentType(fullPath);
                    safeSend(404, "Not Found", body, contentType);
                    return;
                }
            }
        }
    }
}


void Response::handleHttpVersionNotSupported(const std::string &version) {
    std::ostringstream message;
    message << "HTTP Version \"" << version << "\" is not supported. This server only supports "
            << HTTP_SUPPORTED_VERSION << "." << std::endl;
    
    safeSend(505, "HTTP Version Not Supported", message.str(), "text/plain", true);
}

void Response::handleRedirect(const std::string &redirectUrl) {
    LOG_INFO(FGRN("handleRedirect sending HTTP code: 301"));
    std::ostringstream body;
    std::ostringstream response;
    response << "HTTP/1.1 301 Moved Permanently\r\n";
    response << "Location: " << redirectUrl << "\r\n";
    response << "Content-Length: " << body.str().size() << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "\r\n";
    response << body.str();

    std::string responseStr = response.str();
    send(_client_fd, responseStr.c_str(), responseStr.size(), MSG_NOSIGNAL);
}

void Response::addCookie(const std::string &name,
                         const std::string &value,
                         const std::string &path,
                         const std::string &domain,
                         int maxAge)
{
    std::ostringstream cookie;
    cookie << name << "=" << value << "; Path=" << path;
    if (!domain.empty())  cookie << "; Domain=" << domain;
    if (maxAge > 0)       cookie << "; Max-Age=" << maxAge;
    _responseHeaders.emplace_back("Set-Cookie", cookie.str());
}


void Response::setSessionData(const std::string &key, const std::string &value) {
    sessionManager.setSessionData(_sessionId, key, value);
}

std::string Response::getSessionData(const std::string &key) const {
    return sessionManager.getSessionData(_sessionId, key);
}


void Response::cleanup(int _epoll_fd, const std::vector<int> &server_fds) {
    if (_epoll_fd != -1) {
        close(_epoll_fd);
    }
    for (std::vector<int>::const_iterator it = server_fds.begin(); it != server_fds.end(); ++it) {
        close(*it);
    }
    const_cast<std::vector<int>&>(server_fds).shrink_to_fit();
    
}