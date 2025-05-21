#include "ClientRequest.hpp"
#include "DdosProtection.hpp"

ClientRequest::ClientRequest() : 
    _method(""), 
    _path(""), 
    _httpVersion(""), 
    _body(""),
    _resourcePath(""),
    _timeout(0),
    _maxBodySize(REQUEST_MAX_BODY_SIZE),
    _maxUriLength(REQUEST_MAX_URI_LENGTH),
    _timedOut(false)
{}

ClientRequest::~ClientRequest() {}

ClientRequest::ClientRequest( ClientRequest const & other ) {
	(*this) = other;
}

ClientRequest & ClientRequest::operator=( ClientRequest const & other ){
	if (this != &other) {
        this->_method = other._method;
        this->_path = other._path;
        this->_httpVersion = other._httpVersion;
        this->_body = other._body;
        this->_formData = other._formData;
        this->_headers = other._headers;
        this->_timeout = other._timeout;
        this->_maxBodySize = other._maxBodySize;
        this->_maxUriLength = other._maxUriLength;
        this->_timedOut = other._timedOut;
    }
    return (*this);
}

std::string         ClientRequest::getMethod() const { return _method; }
std::string         ClientRequest::getPath() const { return (_path); }
std::string         ClientRequest::getHttpVersion() const { return _httpVersion; }
std::string         ClientRequest::getBody() const { return _body; }
std::map<std::string, std::string> ClientRequest::getHeaders() const { return _headers; }
std::string ClientRequest::getResourcePath() const { return _resourcePath; }
std::map<std::string, std::string> ClientRequest::getQueryParams() const { return _queryParams; }
std::map<std::string, std::string> ClientRequest::getFormData() const { return _formData; }




bool ClientRequest::parseMethod( std::istringstream & stream ) {
    
    std::string line;
    if (!std::getline(stream, line) || line.empty())
        return (false);
        
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
        
    std::istringstream line_stream(line);
    if (!(line_stream >> _method >> _path >> _httpVersion))
        return (false);
    return (true);
}

std::string ClientRequest::getContentType() const {
    for (std::map<std::string, std::string>::const_iterator it = this->_headers.begin(); it != _headers.end(); it++) {
        if (it->first == "Content-Type") {
            return it->second;
        }
    }
    return "/text/plain";
}

void ClientRequest::parseHeaders( std::istringstream & stream ) {
    
    std::string line;
    std::set<std::string> seenHeaders;
    
    while (std::getline(stream, line) && !line.empty()) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
            
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            size_t value_pos = line.find_first_not_of(" \t", colon_pos + 1);
            
            if (key == "Content-Length" || key == "Host" || key == "Content-Type") {
                if (seenHeaders.find(key) != seenHeaders.end()) {
                    _headers.clear();
                    _headers["Invalid-Request"] = "Duplicate " + key + " header";
                    return;
                }
                seenHeaders.insert(key);
            }
            
            if (value_pos != std::string::npos) {
                std::string value = line.substr(value_pos);
                _headers[key] = value;
            }
        }
    }
}

bool ClientRequest::parse( std::string const & rawRequest, DdosProtection* ddosProtector ) {

    if (ddosProtector) {
        ddosProtector->initTimeout();
    }

    std::istringstream request_stream(rawRequest);
    if (!parseMethod(request_stream))
        return (false);
    
    parseHeaders(request_stream);
    
    if (_headers.find("Invalid-Request") != _headers.end()) {
        return (false);
    }

    if (ddosProtector && !ddosProtector->isBodySizeValid(_headers, ddosProtector->getMaxBodySize())) {
        return (false);
    }

    if (ddosProtector) {
        ddosProtector->updateTimeout();
    }
    
    parseBody(request_stream);
    parseContentType();
    parseQueryParams();
    
    return (true);
}


void ClientRequest::parseBody(std::istringstream & request_stream) { 

    if (_headers.find("Content-Length") != _headers.end()) {
        parseContentLengthBody(request_stream);
    }
    else if (_headers.find("Transfer-Encoding") != _headers.end() 
              && _headers["Transfer-Encoding"] == "chunked") {
        parseChunkedBody(request_stream);
    }
}

void ClientRequest::parseContentLengthBody( std::istringstream & request_stream ) {
    
    size_t contentLength = strtoul(_headers["Content-Length"].c_str(), NULL, 10);
    
    if (contentLength > _maxBodySize) {
        return;
    }
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t totalRead = 0;
    _body.clear();
    
    while (totalRead < contentLength) {
        size_t toRead = std::min(bufferSize, contentLength - totalRead);
        request_stream.read(buffer, toRead);
        if (request_stream.eof() || request_stream.bad()) {
            break;
        }
        size_t actualRead = request_stream.gcount();
        
        if (actualRead == 0)
            break;
        
        _body.append(buffer, actualRead);
        totalRead += actualRead;

        if (checkTimeout()) {
            _body.clear();
            return;
        }
    }
}

void ClientRequest::parseChunkedBody(std::istringstream & request_stream) {
    std::string decoded;
    std::string line;
    
    while (std::getline(request_stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
            
        size_t chunk_size = strtoul(line.c_str(), NULL, 16);
        if (chunk_size == 0)
            break;
            
        char* buffer = new char[chunk_size];
        request_stream.read(buffer, chunk_size);
        if (request_stream.eof() or request_stream.bad()) {
            delete[] buffer;
            break;
        }
        decoded.append(buffer, chunk_size);
        
        delete[] buffer;
        
        std::getline(request_stream, line);
    }
    _body = decoded;
}

void ClientRequest::parseContentType() {
    
    if (_headers.find("Content-Type") == _headers.end()) {
        return;
    }
        
    std::string contentType = _headers["Content-Type"];
    
    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        parseFormUrlEncoded();
    }
    else if (contentType.find("multipart/form-data") != std::string::npos) {
        parseMultipartFormData();
    }
    else if (contentType.find("application/json") != std::string::npos) {
        parseJson();
    }
    else if (contentType.find("text/plain") != std::string::npos) {
        parseText();
    }
    else {
        std::cout << "DEBUG: Content-Type not recognized, treating as raw body" << std::endl;
    }
}

void ClientRequest::parseJson() {
    std::map<std::string, std::string> jsonData;
    std::string jsonStr = _body;
        
    size_t start = jsonStr.find('{');
    size_t end = jsonStr.rfind('}');
    
    if (start != std::string::npos && end != std::string::npos && start < end) {
        jsonStr = jsonStr.substr(start + 1, end - start - 1);
        parseJsonContent(jsonStr, jsonData);
        _formData = jsonData;
    }
    else {
        if (start == std::string::npos)
            std::cout << "DEBUG: No opening bracket found" << std::endl;
        if (end == std::string::npos)
            std::cout << "DEBUG: No closing bracket found" << std::endl;
        if (start >= end && start != std::string::npos && end != std::string::npos) 
            std::cout << "DEBUG: Opening bracket after closing bracket" << std::endl;
    }
}

void ClientRequest::parseJsonContent( const std::string & jsonContent, std::map<std::string, std::string>& jsonData ) {
    bool insideQuotes = false;
    std::string currentKey;
    std::string currentValue;
    bool processingKey = true;
    
    for (size_t i = 0; i < jsonContent.length(); i++) {
        char c = jsonContent[i];
        if (c == '"' && (i == 0 || jsonContent[i-1] != '\\')) {
            insideQuotes = !insideQuotes;
            continue;
        }
        if (!insideQuotes) {
            if (isspace(c)) {
                continue;
            }
            if (c == ':' && processingKey) {
                processingKey = false;
                continue;
            }
            if (c == ',') {
                if (!currentKey.empty()) {
                    jsonData[currentKey] = currentValue;
                    currentKey.clear();
                    currentValue.clear();
                }
                processingKey = true;
                continue;
            }
        }
        if (processingKey) {
            if (c != '"') {
                currentKey += c;
            }
        }
        else {
            currentValue += c;
        }
    }
    if (!currentKey.empty()) {
        jsonData[currentKey] = currentValue;
    }
    cleanupJsonValues(jsonData);
}

void ClientRequest::cleanupJsonValues( std::map<std::string, std::string>& jsonData ) {
    for (std::map<std::string, std::string>::iterator it = jsonData.begin(); it != jsonData.end(); ++it) {
        std::string& value = it->second;
        
        size_t start = value.find_first_not_of(" \t\"");
        if (start == std::string::npos) {
            value = "";
            continue;
        }
        
        size_t end = value.find_last_not_of(" \t\"");
        if (end != std::string::npos && end >= start) {
            value = value.substr(start, end - start + 1);
        } else {
            value = "";
        }
    }
}

void ClientRequest::parseText() {
    std::string text = _body;
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        
        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);
            _formData[key] = value;
        }
    }
        if (_formData.empty() && !text.empty()) {
            _formData["content"] = text;
        }
    }
    

void ClientRequest::parseFormUrlEncoded() {
    std::map<std::string, std::string> formData;
    std::string body = _body;
    std::vector<std::string> pairs = splitFormData(body);
    
    for (size_t i = 0; i < pairs.size(); i++) {
        processFormDataPair(pairs[i], formData);
    }
    _formData = formData;
}

std::vector<std::string> ClientRequest::splitFormData( const std::string & data ) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = data.find('&', start)) != std::string::npos) {
        result.push_back(data.substr(start, end - start));
        start = end + 1;
    }
    if (start < data.length()) {
        result.push_back(data.substr(start));
    }
    
    return result;
}

void ClientRequest::processFormDataPair(const std::string& pair, std::map<std::string, std::string>& formData) {
    size_t equalPos = pair.find('=');
    
    if (equalPos != std::string::npos) {
        std::string key = pair.substr(0, equalPos);
        std::string value = pair.substr(equalPos + 1);
        key = urlDecode(key);
        value = urlDecode(value);
        formData[key] = value;
    }
    else if (!pair.empty()) {
        formData[urlDecode(pair)] = "";
    }
}

void ClientRequest::parseMultipartFormData() {
    std::string contentType = _headers["Content-Type"];
    std::string boundary = extractBoundary(contentType);
    if (boundary.empty()) {
        return;
    }
    processMultipartParts(boundary);
}

std::string ClientRequest::extractBoundary(const std::string& contentType) {
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = contentType.find(boundaryPrefix);
    
    if (boundaryPos == std::string::npos) {
        return "";
    }
    return ("--" + contentType.substr(boundaryPos + boundaryPrefix.length()));
}

void ClientRequest::processMultipartParts(const std::string& boundary) {
    size_t position = 0;
    
    while (true) {
        size_t partStart = _body.find(boundary, position);
        if (partStart == std::string::npos) {
            break;
        }
        partStart += boundary.length();
        if (_body.substr(partStart, 2) == "--") {
            break; 
        }
        if (_body.substr(partStart, 2) == "\r\n") {
            partStart += 2;
        }
        size_t partEnd = _body.find(boundary, partStart);
        if (partEnd == std::string::npos) {
            break;
        }
        std::string part = _body.substr(partStart, partEnd - partStart);
        parseMultipartPart(part);
        position = partEnd;
    }
}

void ClientRequest::parseMultipartPart( const std::string& part ) {
    size_t headerEnd = part.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return;
    }
    std::string headers = part.substr(0, headerEnd);
    std::string content = part.substr(headerEnd + 4);
    std::string fieldName = extractFieldName(headers);
    if (!fieldName.empty()) {
        _formData[fieldName] = content;
    }
}

std::string ClientRequest::extractFieldName(const std::string& headers) {
    size_t dispositionPos = headers.find("Content-Disposition:");
    if (dispositionPos == std::string::npos) {
        return "";
    }
    size_t namePos = headers.find("name=", dispositionPos);
    if (namePos == std::string::npos) {
        return "";
    }
    namePos += 6;
    size_t quoteEnd = headers.find("\"", namePos);
    if (quoteEnd == std::string::npos) {
        return "";
    }
    return headers.substr(namePos, quoteEnd - namePos);
}

std::string ClientRequest::urlDecode(const std::string& encoded) {
    std::string decoded;
    size_t i = 0;
    
    while (i < encoded.length()) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            std::string hex = encoded.substr(i + 1, 2);
            int value;
            std::istringstream iss(hex);
            iss >> std::hex >> value;
            decoded += static_cast<char>(value);
            i += 3;
        }
        else if (encoded[i] == '+') {
            decoded += ' ';
            i++;
        }
        else {
            decoded += encoded[i];
            i++;
        }
    }
    return (decoded);
}

void ClientRequest::parseQueryParams() {
    size_t questionPos = _path.find('?');
    if (questionPos == std::string::npos) {
        _resourcePath = _path;
        return;
    }
    _resourcePath = _path.substr(0, questionPos);
    std::string queryString = _path.substr(questionPos + 1);
    size_t pos = 0;
    size_t nextPos;
    
    while (pos < queryString.length()) {
        nextPos = queryString.find('&', pos);
        if (nextPos == std::string::npos) {
            nextPos = queryString.length();
        }
        std::string param = queryString.substr(pos, nextPos - pos);
        size_t equalsPos = param.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = param.substr(0, equalsPos);
            std::string value = param.substr(equalsPos + 1);
            key = urlDecode(key);
            value = urlDecode(value);
            _queryParams[key] = value;
        }
        else {
            std::string key = urlDecode(param);
            _queryParams[key] = "";
        }
        pos = nextPos + 1;
    }
}

void ClientRequest::printRequest() {
    
    std::cout << FYEL("_method: ") << _method << std::endl;
    std::cout << FYEL("_path: ") << _path << std::endl;
    std::cout << FYEL("_httpVersion: ") << _httpVersion << std::endl;
    std::cout << FYEL("_headers:") << std::endl;
        
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
        
    if (!_body.empty()) {
        std::cout << FYEL("_body: ") << _body << std::endl;
    }
    
    std::cout << FYEL("_resourcePath: ") << _resourcePath << std::endl;
    
    if (!_queryParams.empty()) {
        std::cout << FYEL("_queryParams:") << std::endl;
        for (std::map<std::string, std::string>::const_iterator it = _queryParams.begin(); 
             it != _queryParams.end(); ++it) {
            std::cout << "  " << it->first;
            if (!it->second.empty()) {
                std::cout << " = " << it->second;
            }
            std::cout << std::endl;
        }
    }
}

void ClientRequest::initTimeout(time_t seconds) {
    _timeout = time(NULL) + seconds;
}

void ClientRequest::updateTimeout(time_t seconds) {
    _timeout = time(NULL) + seconds;
}

bool ClientRequest::checkTimeout() {
    if (_timeout == 0)
        return false;
        
    if (time(NULL) > _timeout) {
        _timedOut = true;
        return true;
    }
    return false;
}

bool ClientRequest::hasTimedOut() const {
    return _timedOut;
}

void ClientRequest::setMaxBodySize(size_t size) {
    _maxBodySize = size;
}

bool ClientRequest::isBodySizeValid() const {
    if (_headers.find("Content-Length") != _headers.end()) {
        size_t contentLength = strtoul(_headers.at("Content-Length").c_str(), NULL, 10);
        return contentLength <= _maxBodySize;
    }
    return true;
}

bool ClientRequest::isUriTooLong() const {
    return _path.length() > _maxUriLength;
}

bool ClientRequest::isHttpVersionSupported() const {
    return _httpVersion == HTTP_SUPPORTED_VERSION;
}
