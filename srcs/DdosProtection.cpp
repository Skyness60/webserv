/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DdosProtection.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 10:00:00 by okapshai          #+#    #+#             */
/*   Updated: 2025/04/22 13:11:53 by okapshai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "DdosProtection.hpp"

DdosProtection::DdosProtection(int rateWindow, int maxRequests, int blockDuration) :
        _rateWindow(rateWindow),
        _maxRequests(maxRequests),
        _blockDuration(blockDuration),
        _timeout(0),
        _maxBodySize(REQUEST_MAX_BODY_SIZE),
        _timedOut(false) {
}

DdosProtection::DdosProtection(DdosProtection const & other) {
    (*this) = other;
}

DdosProtection & DdosProtection::operator=(DdosProtection const & other) {
    if (this != &other) {
        this->_clientFirstRequestTime = other._clientFirstRequestTime;
        this->_clientRequestCount = other._clientRequestCount;
        this->_clientBlockedStatus = other._clientBlockedStatus;
        this->_clientBlockExpireTime = other._clientBlockExpireTime;
        this->_clientLastActivity = other._clientLastActivity;
        this->_rateWindow = other._rateWindow;
        this->_maxRequests = other._maxRequests;
        this->_blockDuration = other._blockDuration;
        this->_timeout = other._timeout;
        this->_maxBodySize = other._maxBodySize;
        this->_timedOut = other._timedOut;
    }
    return (*this);
}

DdosProtection::~DdosProtection() {}

size_t DdosProtection::getMaxBodySize() const { return _maxBodySize; }

//--------------------------------------------------------------Methods

bool DdosProtection::isClientBlocked(const std::string& clientIp) {
    
    std::map<std::string, bool>::iterator it = _clientBlockedStatus.find(clientIp);
    
    if (it != _clientBlockedStatus.end() && it->second) {

        std::map<std::string, time_t>::iterator expireIt = _clientBlockExpireTime.find(clientIp);
        if (expireIt != _clientBlockExpireTime.end() && std::time(NULL) > expireIt->second) {
            _clientBlockedStatus[clientIp] = false;
            std::cout << "Unblocking client: " << clientIp << std::endl;
            return (false);
        }
        return (true);
    }
    return (false);
}

void DdosProtection::trackClientRequest(const std::string& clientIp) {
    
    time_t currentTime = std::time(NULL);

    if (_clientFirstRequestTime.find(clientIp) == _clientFirstRequestTime.end()) {
        _clientFirstRequestTime[clientIp] = currentTime;
        _clientRequestCount[clientIp] = 1;
        _clientBlockedStatus[clientIp] = false;
        _clientBlockExpireTime[clientIp] = 0;
        _clientLastActivity[clientIp] = currentTime;
        return;
    }

    _clientLastActivity[clientIp] = currentTime;
    
    if (_clientBlockedStatus[clientIp]) {
        return;
    }

    if (currentTime - _clientFirstRequestTime[clientIp] > _rateWindow) {
        _clientFirstRequestTime[clientIp] = currentTime;
        _clientRequestCount[clientIp] = 1;
    }
    else {
        _clientRequestCount[clientIp]++;

        if (_clientRequestCount[clientIp] > _maxRequests) {
            _clientBlockedStatus[clientIp] = true;
            _clientBlockExpireTime[clientIp] = currentTime + _blockDuration;
            std::cout << "Blocking client due to rate limit: " << clientIp << std::endl;
        }
    }
}

void DdosProtection::cleanupOldEntries() {
    time_t currentTime = std::time(NULL);
    std::vector<std::string> keysToRemove;

    for (std::map<std::string, time_t>::iterator it = _clientLastActivity.begin(); 
         it != _clientLastActivity.end(); ++it) {
        
        if (!_clientBlockedStatus[it->first] && (currentTime - it->second > 3600)) {
            keysToRemove.push_back(it->first);
        }
    }

    for (size_t i = 0; i < keysToRemove.size(); i++) {
        _clientFirstRequestTime.erase(keysToRemove[i]);
        _clientRequestCount.erase(keysToRemove[i]);
        _clientBlockedStatus.erase(keysToRemove[i]);
        _clientBlockExpireTime.erase(keysToRemove[i]);
        _clientLastActivity.erase(keysToRemove[i]);
    }
}

bool DdosProtection::isBodySizeValid(const std::map<std::string, std::string>& headers, size_t maxBodySize) {
    
    std::map<std::string, std::string>::const_iterator it = headers.find("Content-Length");
    if (it != headers.end()) {
        size_t contentLength = strtoul(it->second.c_str(), NULL, 10);
        return contentLength <= maxBodySize;
    }
    return (true);
}

void DdosProtection::blockIP(const std::string& clientIp, int durationSeconds) {
    time_t currentTime = std::time(NULL);

    if (_clientFirstRequestTime.find(clientIp) == _clientFirstRequestTime.end()) {
        _clientFirstRequestTime[clientIp] = currentTime;
        _clientRequestCount[clientIp] = 0;
        _clientLastActivity[clientIp] = currentTime;
    }
    
    _clientBlockedStatus[clientIp] = true;
    _clientBlockExpireTime[clientIp] = currentTime + durationSeconds;
    _clientLastActivity[clientIp] = currentTime;

    std::cout << "Manually blocked IP: " << clientIp << " for " << durationSeconds << " seconds" << std::endl;
}

void DdosProtection::unblockIP(const std::string& clientIp) {
    if (_clientBlockedStatus.find(clientIp) != _clientBlockedStatus.end()) {
        _clientBlockedStatus[clientIp] = false;
        std::cout << "Manually unblocked IP: " << clientIp << std::endl;
    }
}

void DdosProtection::configure(int rateWindow, int maxRequests, int blockDuration) {
    _rateWindow = rateWindow;
    _maxRequests = maxRequests;
    _blockDuration = blockDuration;
}

void DdosProtection::initTimeout(time_t seconds) {
    _timeout = time(NULL) + seconds;
}

void DdosProtection::updateTimeout(time_t seconds) {
    _timeout = time(NULL) + seconds;
}

bool DdosProtection::checkTimeout() {
    if (_timeout == 0)
        return (false);

    if (time(NULL) > _timeout) {
        _timedOut = true;
        return (true);
    }
    return (false);
}

void DdosProtection::setMaxBodySize(size_t size) {
    _maxBodySize = size;
}

bool DdosProtection::isBodySizeValid() const {
    return true; 
}

bool DdosProtection::hasTimedOut() const {
    return _timedOut;
}

std::string DdosProtection::getStats() {
    std::string stats = "DDoS Protection Statistics:\n";
    stats += "----------------------------------------\n";
    stats += "Total tracked IPs: " + std::to_string(_clientFirstRequestTime.size()) + "\n";

    int blockedCount = 0;
    for (std::map<std::string, bool>::const_iterator it = _clientBlockedStatus.begin(); 
         it != _clientBlockedStatus.end(); ++it) {
        if (it->second) {
            blockedCount++;
        }
    }

    stats += "Currently blocked IPs: " + std::to_string(blockedCount) + "\n";
    stats += "Rate limit: " + std::to_string(_maxRequests) + " requests per " + std::to_string(_rateWindow) + " seconds\n";
    stats += "Block duration: " + std::to_string(_blockDuration) + " seconds\n";

    return stats;
}

void DdosProtection::testDdosProtection() {
    std::cout << BOLD_CYAN << "\n========== TESTING DDOS PROTECTION ==========\n" << RESET << std::endl;
    
    // Create a DDoS protection instance with test settings
    // 2 second window, 5 max requests, 10 second block duration
    DdosProtection ddosProtector(2, 5, 10);
    
    std::string clientIp = "192.168.1.100";
    std::string clientIp2 = "10.0.0.5";
    
    // TEST 1: Rate limiting functionality
    std::cout << BOLD_GREEN << "TEST 1: Rate limiting functionality" << RESET << std::endl;
    
    // Make 7 requests in quick succession from the same IP
    for (int i = 0; i < 7; i++) {
        std::cout << "Request " << (i + 1) << " from " << clientIp << ": ";
        
        if (ddosProtector.isClientBlocked(clientIp)) {
            std::cout << RED << "BLOCKED" << RESET << std::endl;
        } else {
            std::cout << GREEN << "ALLOWED" << RESET << std::endl;
            ddosProtector.trackClientRequest(clientIp);
        }
        usleep(100000); // 100ms between requests
    }
    
    // TEST 2: Block expiration
    std::cout << BOLD_GREEN << "\nTEST 2: Block expiration" << RESET << std::endl;
    std::cout << "Waiting for block to expire (11 seconds)..." << std::endl;
    sleep(11); // Wait for block to expire
    
    std::cout << "Checking if " << clientIp << " is still blocked: ";
    if (ddosProtector.isClientBlocked(clientIp)) {
        std::cout << RED << "STILL BLOCKED (ERROR)" << RESET << std::endl;
    } else {
        std::cout << GREEN << "UNBLOCKED (SUCCESS)" << RESET << std::endl;
    }
    
    // TEST 3: Manual blocking
    std::cout << BOLD_GREEN << "\nTEST 3: Manual IP blocking" << RESET << std::endl;
    
    // Check initial state
    std::cout << "Initial state of " << clientIp2 << ": ";
    if (ddosProtector.isClientBlocked(clientIp2)) {
        std::cout << RED << "BLOCKED" << RESET << std::endl;
    } else {
        std::cout << GREEN << "ALLOWED" << RESET << std::endl;
    }
    
    // Block the IP manually
    std::cout << "Manually blocking " << clientIp2 << " for 5 seconds..." << std::endl;
    ddosProtector.blockIP(clientIp2, 5);
    
    // Check if blocked
    std::cout << "State after blocking: ";
    if (ddosProtector.isClientBlocked(clientIp2)) {
        std::cout << GREEN << "BLOCKED (SUCCESS)" << RESET << std::endl;
    } else {
        std::cout << RED << "ALLOWED (ERROR)" << RESET << std::endl;
    }
    
    // TEST 4: Body size validation
    std::cout << BOLD_GREEN << "\nTEST 4: Request body size validation" << RESET << std::endl;
    
    // Create a test request with a large content length
    std::map<std::string, std::string> headers;
    headers["Content-Length"] = "20971520"; // 20MB
    
    // Set max body size to 10MB
    ddosProtector.setMaxBodySize(10485760); // 10MB
    
    // Check if body size is valid
    std::cout << "Checking if 20MB request body is allowed with 10MB limit: ";
    if (ddosProtector.isBodySizeValid(headers, ddosProtector.getMaxBodySize())) {
        std::cout << RED << "ALLOWED (ERROR)" << RESET << std::endl;
    } else {
        std::cout << GREEN << "REJECTED (SUCCESS)" << RESET << std::endl;
    }
    
    // TEST 5: Request timeout
    std::cout << BOLD_GREEN << "\nTEST 5: Request timeout" << RESET << std::endl;
    
    // Initialize a timeout of 2 seconds
    ddosProtector.initTimeout(2);
    
    std::cout << "Initialized 2-second timeout. Waiting 3 seconds..." << std::endl;
    sleep(3);
    
    // Check if timed out
    std::cout << "Checking timeout: ";
    if (ddosProtector.checkTimeout()) {
        std::cout << GREEN << "TIMED OUT (SUCCESS)" << RESET << std::endl;
    } else {
        std::cout << RED << "NOT TIMED OUT (ERROR)" << RESET << std::endl;
    }
    
    // Print protection statistics
    std::cout << BOLD_CYAN << "\n========== DDOS PROTECTION STATISTICS ==========\n" << RESET << std::endl;
    std::cout << ddosProtector.getStats() << std::endl;
    
    std::cout << BOLD_CYAN << "========== END OF DDOS PROTECTION TESTS ==========\n" << RESET << std::endl;
}