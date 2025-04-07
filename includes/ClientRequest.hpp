/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientRequest.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/06 15:42:32 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/06 17:50:21 by okapshai         ###   ########.fr       */
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
       

    public:
    
        ClientRequest();
        ClientRequest( ClientRequest const & other);
		ClientRequest & operator=( ClientRequest const & other );
        ~ClientRequest();

        bool                parse( std::string const & rawRequest);
        void                parseBody();
        void                parseFormUrlEncoded();
        void                testClientRequestParsing();
        void                printRequest();


    // Getters
        std::string         getMethod()         const;
        std::string         getPath()           const;
        std::string         getHttpVersion()    const;
        std::string         getBody()           const;
        std::map<std::string, std::string> getHeaders() const;  
        
};