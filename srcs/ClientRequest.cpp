/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:44:39 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/09 13:23:34 by okapshai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientRequest.hpp"

/*HTTP Request = GET*/ 
// GET /index.html HTTP/1.1                     <--------- Method line
// Host: test.com                               <---------- Header line (key : value)
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
// Accept: text/html,application/xhtml+xml
// Accept-Language: en-US,en;q=0.9
// Connection: keep-alive
//                                              <--------- empty line = end of header

/*HTTP Request = POST*/ 
// POST /submit-form HTTP/1.1                   <--------- Method line
// Host: example.com                            <---------- Header line (key : value)
// Content-Type: application/x-www-form-urlencoded  <---------- format of the data in the body
// Content-Length: //length of the body in bytes//

// name=John&age=25&email=john@example.com //   <--------- body: The actual data being sent

/*HTTP Request = DELETE*/ 
// DELETE /users/123 HTTP/1.1                   <--------- resource to delete in the URL path
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

bool ClientRequest::parse( std::string const & rawRequest ) {
    
    std::istringstream request_stream(rawRequest);
    if (!parseMethod(request_stream))
        return (false);
    parseHeaders(request_stream);
    parseBody(request_stream);
    parseContentType();
    
    return (true);
}

void ClientRequest::parseBody( std::istringstream & request_stream ) {

    // Process non-chunked body (Content-Length)
    if (_headers.find("Content-Length") != _headers.end()) {
        size_t content_length = strtoul(_headers["Content-Length"].c_str(), NULL, 10);
        
        char* body_buffer = new char[content_length + 1];
        request_stream.read(body_buffer, content_length);
        body_buffer[content_length] = '\0';
        _body = std::string(body_buffer, content_length);
       
        delete[] body_buffer;
    }

    // Process chunked body (Transfer-Encoding)
    else if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] == "chunked") {
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
        bool insideQuotes = false;
        std::string currentKey;
        std::string currentValue;
        bool processingKey = true;
                
        for (size_t i = 0; i < jsonStr.length(); i++) {
            char c = jsonStr[i];
            if (c == '"' && (i == 0 || jsonStr[i-1] != '\\')) {
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
        
        for (std::map<std::string, std::string>::iterator it = jsonData.begin(); it != jsonData.end(); ++it) {
            std::string& value = it->second;
            value = value.substr(value.find_first_not_of(" \t\""));
            size_t end = value.find_last_not_of(" \t\"");
            if (end != std::string::npos) {
                value = value.substr(0, end + 1);
            }
        }
        _formData = jsonData;
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
    std::string key;
    std::string value;
    std::istringstream stream(_body);
    
    while (std::getline(stream, key, '=') && std::getline(stream, value, '&')) {
        formData[key] = value;
    }
    _formData = formData;
}

void ClientRequest::parseMultipartFormData() {

    std::string contentType = _headers["Content-Type"];
    std::string boundaryPrefix = "boundary=";
    size_t boundaryPos = contentType.find(boundaryPrefix);
    if (boundaryPos == std::string::npos)
        return;
    std::string boundary = "--" + contentType.substr(boundaryPos + boundaryPrefix.length());
    
    size_t i = 0;
    while (true) {
        
        size_t start = _body.find(boundary, i);
        if (start == std::string::npos)
            break;
        start += boundary.length();
        if (_body.substr(start, 2) == "--")
            break;
        if (_body.substr(start, 2) == "\r\n")
            start += 2;
        size_t end = _body.find(boundary, start);
        if (end == std::string::npos)
            break;
        
        std::string part = _body.substr(start, end - start);
        
        size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headersPart = part.substr(0, headerEnd);
            std::string content = part.substr(headerEnd + 4);
            
            size_t dispositionPos = headersPart.find("Content-Disposition:");
            if (dispositionPos != std::string::npos) {
                size_t namePos = headersPart.find("name=", dispositionPos);
                if (namePos != std::string::npos) {
                    namePos += 6;
                    size_t quoteEnd = headersPart.find("\"", namePos);
                    if (quoteEnd != std::string::npos) {
                        std::string fieldName = headersPart.substr(namePos, quoteEnd - namePos);
                        _formData[fieldName] = content;
                    }
                }
            }
        }
        i = end;
    }
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
    
    std::cout << FBLU("\n======== Testing POST Method ========\n") << std::endl;
    std::string testPostRequest = 
        "POST /submit-form HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "username=john&password=secret";
    
    ClientRequest postRequest;
    bool postParseSuccess = postRequest.parse(testPostRequest);
    
    
    std::cout << "Parsing result: " << (postParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (postParseSuccess) {
        postRequest.printRequest();
    }

    std::cout << FBLU("\n======== Testing Form URL Encoded Parsing ========\n") << std::endl;
    std::string testFormUrlEncodedRequest = 
        "POST /submit-form HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 44\r\n"
        "\r\n"
        "username=johndoe&email=john@example.com&age=30";

    ClientRequest formUrlEncodedRequest;
    bool formUrlEncodedParseSuccess = formUrlEncodedRequest.parse(testFormUrlEncodedRequest);

    std::cout << "Parsing result: " << (formUrlEncodedParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (formUrlEncodedParseSuccess) {
        formUrlEncodedRequest.printRequest();

        std::cout << FYEL("Parsed Form URL Encoded data:") << std::endl;
        std::map<std::string, std::string> formData = formUrlEncodedRequest._formData;
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
            it != formData.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }

    std::cout << FBLU("\n======== Testing Multipart Form Data Parsing ========\n") << std::endl;
        std::string boundary = "----WebKitFormBoundaryX7YA15VlAH";
        std::string multipartBody = 
            "--" + boundary + "\r\n" +
            "Content-Disposition: form-data; name=\"username\"\r\n" +
            "\r\n" +
            "janedoe\r\n" +
            "--" + boundary + "\r\n" +
            "Content-Disposition: form-data; name=\"email\"\r\n" +
            "\r\n" +
            "jane@example.com\r\n" +
            "--" + boundary + "\r\n" +
            "Content-Disposition: form-data; name=\"age\"\r\n" +
            "\r\n" +
            "28\r\n" +
            "--" + boundary + "--\r\n";

        std::string testMultipartRequest = 
            "POST /upload HTTP/1.1\r\n"
            "Host: localhost:8080\r\n"
            "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
            "Content-Length: " + static_cast<std::ostringstream*>(&(std::ostringstream() << multipartBody.length()))->str() + "\r\n"
            "\r\n" +
            multipartBody;

    ClientRequest multipartRequest;
    bool multipartParseSuccess = multipartRequest.parse(testMultipartRequest);

    std::cout << "Parsing result: " << (multipartParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (multipartParseSuccess) {
        multipartRequest.printRequest();
        
        // Display the parsed form data
        std::cout << FYEL("Parsed Multipart Form data:") << std::endl;
        std::map<std::string, std::string> formData = multipartRequest._formData;
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
            it != formData.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }

    std::cout << FBLU("\n======== Testing JSON Parsing ========\n") << std::endl;
    std::string testJsonRequest = 
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 59\r\n"
        "\r\n"
        "{\"name\":\"John Doe\",\"email\":\"john@example.com\",\"age\":\"30\"}";
    
    ClientRequest jsonRequest;
    bool jsonParseSuccess = jsonRequest.parse(testJsonRequest);
    
    std::cout << "Parsing result: " << (jsonParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (jsonParseSuccess) {
        jsonRequest.printRequest();
        
        // Display the parsed form data
        std::cout << FYEL("Parsed JSON data:") << std::endl;
        std::map<std::string, std::string> formData = jsonRequest._formData;
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
             it != formData.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }
    
    // Test for Text parsing
    std::cout << FBLU("\n======== Testing Text Parsing ========\n") << std::endl;
    std::string testTextRequest = 
        "POST /api/text HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 46\r\n"
        "\r\n"
        "name=Jane Doe\r\nemail=jane@example.com\r\nage=28";
    
    ClientRequest textRequest;
    bool textParseSuccess = textRequest.parse(testTextRequest);
    
    std::cout << "Parsing result: " << (textParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (textParseSuccess) {
        textRequest.printRequest();
        
        // Display the parsed form data
        std::cout << FYEL("Parsed Text data:") << std::endl;
        std::map<std::string, std::string> formData = textRequest._formData;
        for (std::map<std::string, std::string>::const_iterator it = formData.begin(); 
             it != formData.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }
    

    std::cout << FBLU("\n======== Testing POST CHUNCKED Method ========\n") << std::endl;
    std::string testChunkedRequest = 
    "POST /chunked HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "7\r\n"
    "Mozilla\r\n"
    "9\r\n"
    "Developer\r\n"
    "7\r\n"
    "Network\r\n"
    "0\r\n"
    "\r\n";

    ClientRequest chunkedRequest;
    bool chunkedParseSuccess = chunkedRequest.parse(testChunkedRequest);

    std::cout << "Parsing result: " << (chunkedParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (chunkedParseSuccess) {
        chunkedRequest.printRequest();
    }
    
    std::cout << FBLU("\n======== Testing DELETE Method ========\n") << std::endl;
    std::string testDeleteRequest = 
        "DELETE /users/123 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Authorization: Bearer token123\r\n"
        "\r\n";
    
    ClientRequest deleteRequest;
    bool deleteParseSuccess = deleteRequest.parse(testDeleteRequest);
    
    std::cout << "Parsing result: " << (deleteParseSuccess ? FGRN("SUCCESS") : FRED("FAILED")) << std::endl;
    if (deleteParseSuccess) {
        deleteRequest.printRequest();
    }
}