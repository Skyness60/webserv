/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:42:32 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/29 17:23:40 by okapshai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Includes.hpp"

class DdosProtection;

#define REQUEST_DEFAULT_HEADER_TIMEOUT 10  // 10 seconds for headers
#define REQUEST_DEFAULT_BODY_TIMEOUT 30    // 30 seconds for body
#define REQUEST_MAX_BODY_SIZE 10485760     // 10MB default max body size

class ClientRequest {
    
    private:
    
        std::string         _method;
        std::string         _path;
        std::string         _httpVersion;
        std::string         _body;
        std::map<std::string, std::string>  _formData;
        std::map<std::string, std::string>  _headers;
        std::map<std::string, std::string>  _queryParams;  
        std::string         _resourcePath;
        time_t              _timeout;
        size_t              _maxBodySize;
        bool                _timedOut;                 

    public:
    
        ClientRequest();
        ClientRequest( ClientRequest const & other);
        ClientRequest & operator=( ClientRequest const & other );
        ~ClientRequest();

        bool                parse( std::string const & rawRequest, DdosProtection* ddosProtector);
        bool                parseMethod( std::istringstream & stream );
        void                parseHeaders( std::istringstream & stream );
        void                parseBody( std::istringstream & request_stream );
        void                parseContentLengthBody( std::istringstream & request_stream );
        void                parseChunkedBody( std::istringstream & request_stream );
        void                parseContentType();
        void                parseFormUrlEncoded();
        std::vector<std::string> splitFormData(const std::string& data);
        void                processFormDataPair( const std::string & pair, std::map<std::string, std::string> & formData );
        void                parseMultipartFormData();
        void                processMultipartParts( const std::string & boundary );
        std::string         extractFieldName( const std::string & headers );
        void                parseMultipartPart( const std::string& part ) ;
        std::string         extractBoundary( const std::string & contentType );
        void                parseJson();
        void                parseJsonContent( const std::string& jsonContent, std::map<std::string, std::string>& jsonData);
        void                cleanupJsonValues( std::map<std::string, std::string>& jsonData );
        void                parseText();  
        void                testClientRequestParsing();
        void                printRequest();
        void                parseQueryParams();
        std::string         urlDecode(const std::string& encoded);
        void                initTimeout(time_t seconds = REQUEST_DEFAULT_HEADER_TIMEOUT);
        void                updateTimeout(time_t seconds = REQUEST_DEFAULT_BODY_TIMEOUT);
        bool                checkTimeout();
        void                setMaxBodySize(size_t size);
        bool                isBodySizeValid() const;
        bool                hasTimedOut() const;

    // Getters
        std::string         getMethod()         const;
        std::string         getPath()           const;
		std::map<std::string, std::string> getFormData() const;
        std::string         getResourcePath()   const;
        std::string         getHttpVersion()    const;
        std::string         getBody()           const;
        std::map<std::string, std::string> getHeaders() const;
        std::map<std::string, std::string> getQueryParams() const;
        
};