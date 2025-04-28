/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:44:39 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/22 12:47:55 by okapshai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientRequest.hpp"
#include "DdosProtection.hpp"

ClientRequest::ClientRequest() : 
    _method(""), 
    _path(""), 
    _httpVersion(""), 
    _body(""),
    _resourcePath("")
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
    }
    return (*this);
}

std::string         ClientRequest::getMethod() const { return _method; }
std::string         ClientRequest::getPath() const { return _path; }
std::string         ClientRequest::getHttpVersion() const { return _httpVersion; }
std::string         ClientRequest::getBody() const { return _body; }
std::map<std::string, std::string> ClientRequest::getHeaders() const { return _headers; }
std::string ClientRequest::getResourcePath() const { return _resourcePath; }
std::map<std::string, std::string> ClientRequest::getQueryParams() const { return _queryParams; }
std::map<std::string, std::string> ClientRequest::getFormData() const { return _formData; }

//--------------------------------------------------------------Methods


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

void ClientRequest::parseHeaders( std::istringstream & stream ) {
    
    std::string line;
    while (std::getline(stream, line) && !line.empty()) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
            
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            size_t value_pos = line.find_first_not_of(" \t", colon_pos + 1);
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
    
    const size_t bufferSize = 8192; // 8KB chunks
    char buffer[bufferSize];
    size_t totalRead = 0;
    _body.clear();
    
    while (totalRead < contentLength) {
        size_t toRead = std::min(bufferSize, contentLength - totalRead);
        request_stream.read(buffer, toRead);
        size_t actualRead = request_stream.gcount();
        
        if (actualRead == 0)
            break;
        
        _body.append(buffer, actualRead);
        totalRead += actualRead;
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
        decoded.append(buffer, chunk_size);
        
        delete[] buffer;
        
        std::getline(request_stream, line);
    }
    _body = decoded;
}

void ClientRequest::parseContentType() {
    
    if (_headers.find("Content-Type") == _headers.end())
        return;
        
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
        value = value.substr(value.find_first_not_of(" \t\""));
        size_t end = value.find_last_not_of(" \t\"");
        if (end != std::string::npos) {
            value = value.substr(0, end + 1);
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
    std::vector<std::string> pairs = splitFormData(body);// Split by '&' to get key-value pairs
    
    for (size_t i = 0; i < pairs.size(); i++) {
        processFormDataPair(pairs[i], formData);
    }
    _formData = formData;
    // std::cout << FYEL("_formData: ") << std::endl;
    // for (std::map<std::string, std::string>::const_iterator it = _formData.begin(); it != _formData.end(); ++it) {
    //     std::cout << "  " << it->first << ": " << it->second << std::endl;
    // }
}

std::vector<std::string> ClientRequest::splitFormData( const std::string & data ) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = 0;
    
    while ((end = data.find('&', start)) != std::string::npos) {
        result.push_back(data.substr(start, end - start));
        start = end + 1;
    }
    if (start < data.length()) {// Add the last part
        result.push_back(data.substr(start));
    }
    
    return result;
}

void ClientRequest::processFormDataPair(const std::string& pair, std::map<std::string, std::string>& formData) {
    size_t equalPos = pair.find('=');
    
    if (equalPos != std::string::npos) {
        std::string key = pair.substr(0, equalPos);
        std::string value = pair.substr(equalPos + 1);
        key = urlDecode(key);// URL decode key and value
        value = urlDecode(value);
        formData[key] = value;
    }
    else if (!pair.empty()) {// Handle case where there's a key but no value
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
            break; // End boundary marker
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
    namePos += 6; // Skip "name=\""
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

void ClientRequest::testClientRequestParsing() {

    // std::cout << FBLU("\n======== Testing GET Method ========\n") << std::endl;
    // std::string testRequest = 
    //     "GET /index.html HTTP/1.1\r\n"
    //     "Host: localhost:8080\r\n"
    //     "User-Agent: Mozilla/5.0\r\n"
    //     "Accept: text/html,application/xhtml+xml\r\n"
    //     "Connection: keep-alive\r\n"
    //     "\r\n";
    
    // ClientRequest request;
    // bool parseSuccess = request.parse(testRequest);
    
    // std::cout << "Parsing result: " << (parseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    // if (parseSuccess) {
    //     request.printRequest();
    // }
    
    // std::cout << FBLU("\n======== Testing POST Method ========\n") << std::endl;
    // std::string testPostRequest = 
    //     "POST /submit-form HTTP/1.1\r\n"
    //     "Host: localhost:8080\r\n"
    //     "Content-Type: application/x-www-form-urlencoded\r\n"
    //     "Content-Length: 29\r\n"
    //     "\r\n"
    //     "username=john&password=secret";
    
    // ClientRequest postRequest;
    // bool postParseSuccess = postRequest.parse(testPostRequest);
    
    
    // std::cout << "Parsing result: " << (postParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    // if (postParseSuccess) {
    //     postRequest.printRequest();
        
    //     std::cout << FYEL("_formData:") << std::endl;
    //     std::map<std::string, std::string> formData = postRequest._formData;
    //     for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
    //         it != formData.end(); ++it) {
    //         std::cout << "  " << it->first << ": " << it->second << std::endl;
    //     }
    // }

    // std::cout << FBLU("\n======== Testing Form URL Encoded Parsing ========\n") << std::endl;
    // std::string testFormUrlEncodedRequest = 
    //     "POST /submit-form HTTP/1.1\r\n"
    //     "Host: localhost:8080\r\n"
    //     "Content-Type: application/x-www-form-urlencoded\r\n"
    //     "Content-Length: 46\r\n"
    //     "\r\n"
    //     "username=johndoe&email=john@example.com&age=30";

    // ClientRequest formUrlEncodedRequest;
    // bool formUrlEncodedParseSuccess = formUrlEncodedRequest.parse(testFormUrlEncodedRequest);

    // std::cout << "Parsing result: " << (formUrlEncodedParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    // if (formUrlEncodedParseSuccess) {
    //     formUrlEncodedRequest.printRequest();

    //     std::cout << FYEL("_formData:") << std::endl;
    //     std::map<std::string, std::string> formData = formUrlEncodedRequest._formData;
    //     for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
    //         it != formData.end(); ++it) {
    //         std::cout << "  " << it->first << ": " << it->second << std::endl;
    //     }
    // }

//     std::cout << FBLU("\n======== Testing JSON Parsing ========\n") << std::endl;
//     std::string testJsonRequest = 
//         "POST /api/data HTTP/1.1\r\n"
//         "Host: localhost:8080\r\n"
//         "Content-Type: application/json\r\n"
//         "Content-Length: 59\r\n"
//         "\r\n"
//         "{\"name\":\"John Doe\",\"email\":\"john@example.com\",\"age\":\"30\"}";
    
//     ClientRequest jsonRequest;
//     bool jsonParseSuccess = jsonRequest.parse(testJsonRequest);
    
//     std::cout << "Parsing result: " << (jsonParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
//     if (jsonParseSuccess) {
//         jsonRequest.printRequest();
        
//         std::cout << FYEL("Parsed JSON data:") << std::endl;
//         std::map<std::string, std::string> formData = jsonRequest._formData;
//         for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
//              it != formData.end(); ++it) {
//             std::cout << "  " << it->first << ": " << it->second << std::endl;
//         }
//     }
    
//     std::cout << FBLU("\n======== Testing Text Parsing ========\n") << std::endl;
//     std::string testTextRequest = 
//         "POST /api/text HTTP/1.1\r\n"
//         "Host: localhost:8080\r\n"
//         "Content-Type: text/plain\r\n"
//         "Content-Length: 46\r\n"
//         "\r\n"
//         "name=Jane Doe\r\nemail=jane@example.com\r\nage=28";
    
//     ClientRequest textRequest;
//     bool textParseSuccess = textRequest.parse(testTextRequest);
    
//     std::cout << "Parsing result: " << (textParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
//     if (textParseSuccess) {
//         textRequest.printRequest();
//     }
    

//     std::cout << FBLU("\n======== Testing POST CHUNCKED Method ========\n") << std::endl;
//     std::string testChunkedRequest = 
//     "POST /chunked HTTP/1.1\r\n"
//     "Host: localhost:8080\r\n"
//     "Transfer-Encoding: chunked\r\n"
//     "Content-Type: text/plain\r\n"
//     "\r\n"
//     "7\r\n"
//     "Mozilla\r\n"
//     "9\r\n"
//     "Developer\r\n"
//     "7\r\n"
//     "Network\r\n"
//     "0\r\n"
//     "\r\n";

//     ClientRequest chunkedRequest;
//     bool chunkedParseSuccess = chunkedRequest.parse(testChunkedRequest);

//     std::cout << "Parsing result: " << (chunkedParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
//     if (chunkedParseSuccess) {
//         chunkedRequest.printRequest();
//     }
    
//     std::cout << FBLU("\n======== Testing DELETE Method ========\n") << std::endl;
//     std::string testDeleteRequest = 
//         "DELETE /users/123 HTTP/1.1\r\n"
//         "Host: localhost:8080\r\n"
//         "Authorization: Bearer token123\r\n"
//         "\r\n";
    
//     ClientRequest deleteRequest;
//     bool deleteParseSuccess = deleteRequest.parse(testDeleteRequest);
    
//     std::cout << "Parsing result: " << (deleteParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
//     if (deleteParseSuccess) {
//         deleteRequest.printRequest();
//     }

    // std::cout << FBLU("\n======== Testing URL Query Parameter Parsing ========\n") << std::endl;
    // std::string testUrlRequest = 
    //     "GET /search?q=test%20query&page=2&sort=date&filter=active HTTP/1.1\r\n"
    //     "Host: localhost:8080\r\n"
    //     "Connection: keep-alive\r\n"
    //     "\r\n";

    // ClientRequest urlRequest;
    // bool urlParseSuccess = urlRequest.parse(testUrlRequest);

    // std::cout << "Parsing result: " << (urlParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    // if (urlParseSuccess) {
    //     urlRequest.printRequest();
    // }

    // Test DoS Protection using DdosProtection class instead of inline code
    std::cout << FBLU("\n======== Testing DoS Protection ========\n") << std::endl;
    
    // Create an instance of DdosProtection for testing
    DdosProtection ddosProtector(60, 100, 300);
    
    std::string largeBodyTestRequest = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 104857600\r\n" // 100MB (should exceed default limit)
        "\r\n";
    
    ClientRequest largeBodyRequest;
    bool largeBodyResult = largeBodyRequest.parse(largeBodyTestRequest, &ddosProtector);
    
    std::cout << "Large body protection test: " << 
        (largeBodyResult ? FRED("FAILED - Large body allowed") : FGRN("SUCCESS - Large body rejected")) << std::endl;
    
    // Test rate limiting
    std::cout << FBLU("\n======== Testing Rate Limiting ========\n") << std::endl;
    
    // Simulate multiple requests from same IP
    std::string clientIp = "192.168.1.100";
    
    // Configure with strict rate limiting for testing
    ddosProtector.configure(1, 3, 10); // 3 requests per second, 10 second block
    
    for (int i = 0; i < 5; i++) {
        // Track a request
        ddosProtector.trackClientRequest(clientIp);
        
        // Check if blocked after tracking
        bool blocked = ddosProtector.isClientBlocked(clientIp);
        
        std::cout << "Request " << (i+1) << ": " << 
            (blocked ? FRED("BLOCKED") : FGRN("ALLOWED")) << std::endl;
    }
    
    // Print DDoS protection stats
    std::cout << FBLU("\n======== DDoS Protection Statistics ========\n") << std::endl;
    std::cout << ddosProtector.getStats() << std::endl;
}

