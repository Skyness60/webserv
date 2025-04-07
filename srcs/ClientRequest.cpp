/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:44:39 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/06 17:59:16 by okapshai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientRequest.hpp"

/*HTTP Reques = GET*/ 
// GET /index.html HTTP/1.1                 <--------- Method line
// Host: test.com                           <---------- Header line (key : value)
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
// Accept: text/html,application/xhtml+xml
// Accept-Language: en-US,en;q=0.9
// Connection: keep-alive
//                                          <--------- emty line = end of header

/*HTTP Reques = POST*/ 
// POST /submit-form HTTP/1.1               <--------- Method line
// Host: example.com                        <---------- Header line (key : value)
// Content-Type: application/x-www-form-urlencoded  <---------- format of the data in the body
// Content-Length: //length of the body in bytes//

// name=John&age=25&email=john@example.com // <--------- body: The actual data being sent

/*HTTP Reques = DELETE*/ 
// DELETE /users/123 HTTP/1.1               <--------- resource to delete in the URL path
// Host: example.com
// Authorization: Bearer token123              

ClientRequest::ClientRequest() {}

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

//--------------------------------------------------------------Methods


bool ClientRequest::parse( std::string const & rawRequest ) {
    
    std::istringstream request_stream(rawRequest);
    std::string line;
    
    // Parse the method line
    if (!std::getline(request_stream, line) || line.empty()) {
        return false;
    }
    if (line[line.length() - 1] == '\r') {
        line = line.substr(0, line.length() - 1);
    }
        
    std::istringstream line_stream(line);
    if (!(line_stream >> _method >> _path >> _httpVersion)) {
        return false;
    }
        
    // Parse header
    while (std::getline(request_stream, line) && !line.empty()) {
        
        if (line[line.length() - 1] == '\r') {
            line = line.substr(0, line.length() - 1);
        }
        if (line.empty()) {
                break;
        }
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
        
        // Parse body (for POST )
        if (_headers.find("Content-Length") != _headers.end()) {
            size_t content_length = std::strtoul(_headers["Content-Length"].c_str(), NULL, 10);
            char body_buffer[content_length + 1];
            request_stream.read(body_buffer, content_length);
            body_buffer[content_length] = '\0';
            _body = std::string(body_buffer, content_length);
        }
        return (true);
}

void ClientRequest::parseBody() {
    
    if (_headers.find("Content-Length") == _headers.end() && 
        _headers.find("Transfer-Encoding") == _headers.end()) {
        return;
    }

    if (_headers.find("Transfer-Encoding") != _headers.end() && 
        _headers["Transfer-Encoding"] == "chunked") {
        // TO DO
    }

    else if (_headers.find("Content-Length") != _headers.end()) {
        // TO DO
    }
    
    if (_headers.find("Content-Type") != _headers.end()) {
        std::string contentType = _headers["Content-Type"];
        
        if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
            parseFormUrlEncoded();
        }
        else if (contentType.find("multipart/form-data") != std::string::npos) {
            //parseMultipartFormData();
        }
        else if (contentType.find("application/json") != std::string::npos) {
            //parseJson();
        }
        else if (contentType.find("text/plain") != std::string::npos) {
            //parseText();
        }
    }
}

void ClientRequest::parseFormUrlEncoded() {
    
    std::map<std::string, std::string> formData;
    std::string key;
    std::string value;
    std::istringstream stream(_body);
    
    while (std::getline(stream, key, '=') && std::getline(stream, value, '&')) {
        formData[key] = value;
    }
    _formData = formData;
}

void ClientRequest::printRequest() {
    
    std::cout << FYEL("Method: ") << _method << std::endl;
    std::cout << FYEL("Path: ") << _path << std::endl;
    std::cout << FYEL("HTTP Version: ") << _httpVersion << std::endl;
    std::cout << FYEL("Headers:") << std::endl;
        
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it) {
        std::cout << "  " << it->first << ": " << it->second << std::endl;
    }
        
    if (!_body.empty()) {
        std::cout << FYEL("Body: ") << _body << std::endl;
    }
}

void ClientRequest::testClientRequestParsing() {

    std::cout << FBLU("\n======== Testing GET Method ========\n") << std::endl;
    
    std::string testRequest = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html,application/xhtml+xml\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    ClientRequest request;
    bool parseSuccess = request.parse(testRequest);
    
    std::cout << "Parsing result: " << (parseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (parseSuccess) {
        request.printRequest();
    }
    
    std::string testPostRequest = 
        "POST /submit-form HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=john&password=secret";
    
    ClientRequest postRequest;
    bool postParseSuccess = postRequest.parse(testPostRequest);
    
    std::cout << FBLU("\n======== Testing POST Method ========\n") << std::endl;
    std::cout << "Parsing result: " << (postParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (postParseSuccess) {
        postRequest.printRequest();
    }
    
    std::string testDeleteRequest = 
        "DELETE /users/123 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Authorization: Bearer token123\r\n"
        "\r\n";
    
    ClientRequest deleteRequest;
    bool deleteParseSuccess = deleteRequest.parse(testDeleteRequest);
    
    std::cout << FBLU("\n======== Testing DELETE Method ========\n") << std::endl;
    std::cout << "Parsing result: " << (deleteParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (deleteParseSuccess) {
        deleteRequest.printRequest();
    }
}