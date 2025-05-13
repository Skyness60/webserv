/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DdosProtection.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: olly <olly@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 11:57:36 by okapshai          #+#    #+#             */
/*   Updated: 2025/05/13 14:28:19 by olly             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Includes.hpp"

#define REQUEST_DEFAULT_HEADER_TIMEOUT 10  // 10 seconds for headers
#define REQUEST_DEFAULT_BODY_TIMEOUT 30    // 30 seconds for body
#define REQUEST_MAX_BODY_SIZE 104857600    // 100MB default max body size for ubuntu_tester compatibility
#define DDOS_DEFAULT_RATE_WINDOW 60        // 60 seconds time window
#define DDOS_DEFAULT_MAX_REQUESTS 100      // 100 max requests in the window
#define DDOS_DEFAULT_BLOCK_DURATION 300    // 300 seconds (5 minutes) block duration

class DdosProtection {
    
    private:

        protected:
            std::map<std::string, time_t> _clientFirstRequestTime;
            std::map<std::string, int> _clientRequestCount;
            std::map<std::string, bool> _clientBlockedStatus;
            std::map<std::string, time_t> _clientBlockExpireTime;
            std::map<std::string, time_t> _clientLastActivity;

            int             _rateWindow;
            int             _maxRequests;
            int             _blockDuration;
            time_t          _timeout;
            size_t          _maxBodySize;
            bool            _timedOut;

        public:
    
        DdosProtection(int rateWindow, int maxRequests, int blockDuration);
        DdosProtection( DdosProtection const & other);
        DdosProtection & operator=( DdosProtection const & other );
        ~DdosProtection();

        size_t          getMaxBodySize() const;
        bool            isClientBlocked(const std::string& clientIp);
        void            trackClientRequest(const std::string& clientIp);
        void            cleanupOldEntries();
        bool            isBodySizeValid(const std::map<std::string, std::string>& headers, size_t maxBodySize);
        void            blockIP(const std::string& clientIp, int durationSeconds);
        void            unblockIP(const std::string& clientIp);
        std::string     getStats();
        void            configure(int rateWindow, int maxRequests, int blockDuration);
        void            initTimeout(time_t seconds = REQUEST_DEFAULT_HEADER_TIMEOUT);
        void            updateTimeout(time_t seconds = REQUEST_DEFAULT_BODY_TIMEOUT);
        bool            checkTimeout();
        void            setMaxBodySize(size_t size);
        bool            isBodySizeValid() const;
        bool            hasTimedOut() const;
        void            testDdosProtection();
};