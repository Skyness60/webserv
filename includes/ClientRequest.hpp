/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: olly <olly@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:42:32 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/11 15:32:55 by olly             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Includes.hpp"

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
       

    public:
    
        ClientRequest();
        ClientRequest( ClientRequest const & other);
        ClientRequest & operator=( ClientRequest const & other );
        ~ClientRequest();

        bool                parse( std::string const & rawRequest);
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