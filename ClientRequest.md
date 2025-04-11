# Client Request Handling Strategy
## Overview

The client request parser follows a sequential strategy to process incoming HTTP requests:

1. 👉 Parse the request method line
2. 👉 Parse HTTP headers
3. 👉 Parse request body (for POST methode)
4. 👉 Process content based on Content-Type
5. 👉 Parse query parameters (if present in URL)

## Implementation Status

### Core HTTP Parsing
- ✅ Method line parsing
- ✅ Headers parsing
- ✅ Body extraction

### Content Type Handling
- ✅ application/x-www-form-urlencoded
- ✅ multipart/form-data
- ✅ application/json
- ✅ text/plain

### Advanced Features
- ✅ URL query parameter parsing
- ✅ Chunked transfer encoding
- ⏳ File upload handling (basic implementation complete, advanced features in progress)
- ⏳ Request validation and sanitization

## Future Improvements

1. ⏳ Better error handling for malformed requests
2. ⏳ Support for additional content types
3. ⏳ Request size limits to prevent DoS attacks
4. ⏳ Enhanced validation of input data


## Function Descriptions

### Main Parsing Functions

#### `parse(std::string const & rawRequest)` ✅
The main entry point for parsing an HTTP request. Orchestrates the entire parsing process.

**Example Input:**
```
GET /index.html?page=1 HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0
Connection: keep-alive

```

#### `parseMethod(std::istringstream & stream)` ✅
Parses the HTTP method line to extract the method, path, and HTTP version.

**Example Input:**
```
GET /index.html HTTP/1.1
```

**What it does:**
- Extracts `_method` (GET)
- Extracts `_path` (/index.html)
- Extracts `_httpVersion` (HTTP/1.1)

#### `parseHeaders(std::istringstream & stream)` ✅
Parses HTTP headers into a key-value map.

**Example Input:**
```
Host: localhost:8080
User-Agent: Mozilla/5.0
Content-Type: application/json
Content-Length: 27
```

**What it does:**
- Populates `_headers` map with header keys and values

#### `parseBody(std::istringstream & request_stream)` ✅
Handles body extraction based on Content-Length or chunked transfer encoding.

**Example Input for Content-Length:**
```
{"name":"John","age":"30"}
```

**Example Input for Chunked Transfer:**
```
7
Mozilla
9
Developer
7
Network
0

```

### Body Parsing Functions

#### `parseContentLengthBody(std::istringstream & request_stream)` ✅
Extracts the body using the Content-Length header.

#### `parseChunkedBody(std::istringstream & request_stream)` ✅
Processes a body with chunked transfer encoding format.

#### `parseContentType()` ✅
Determines the body content type and calls the appropriate parser.

#### `parseFormUrlEncoded()` ✅
Parses application/x-www-form-urlencoded bodies.

**Example Input:**
```
username=johndoe&email=john@example.com&age=30
```

#### `parseMultipartFormData()` ✅
Parses multipart/form-data bodies, typically used for file uploads.

**Example Input:**
```
----WebKitFormBoundaryX7YA15VlAH
Content-Disposition: form-data; name="username"

janedoe
----WebKitFormBoundaryX7YA15VlAH
Content-Disposition: form-data; name="email"

jane@example.com
----WebKitFormBoundaryX7YA15VlAH--
```

#### `parseJson()` ✅
Parses application/json bodies.

**Example Input:**
```
{"name":"John Doe","email":"john@example.com","age":"30"}
```

#### `parseText()` ✅
Parses text/plain bodies.

**Example Input:**
```
name=Jane Doe
email=jane@example.com
age=28
```

### Helper Functions

#### `extractBoundary(const std::string& contentType)` ✅
Extracts boundary string from Content-Type header for multipart/form-data.

#### `processMultipartParts(const std::string& boundary)` ✅
Processes individual parts in multipart/form-data.

#### `parseMultipartPart(const std::string& part)` ✅
Parses a single part from multipart/form-data.

#### `extractFieldName(const std::string& headers)` ✅
Extracts field name from Content-Disposition header.

#### `parseJsonContent(const std::string& jsonContent, std::map<std::string, std::string>& jsonData)` ✅
Processes JSON content into key-value pairs.

#### `cleanupJsonValues(std::map<std::string, std::string>& jsonData)` ✅
Cleans up JSON values by removing quotes and whitespace.

#### `splitFormData(const std::string& data)` ✅
Splits form URL encoded data into key-value pairs.

#### `processFormDataPair(const std::string& pair, std::map<std::string, std::string>& formData)` ✅
Processes a single key-value pair in form URL encoded data.

#### `urlDecode(const std::string& encoded)` ✅
Decodes URL-encoded characters (like %20 for spaces).

#### `parseQueryParams()` ✅
Extracts and processes query parameters from the URL.

