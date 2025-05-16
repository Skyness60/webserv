/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   DdosProtection.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: okapshai <okapshai@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 10:00:00 by okapshai          #+#    #+#             */
/*   Updated: 2025/05/16 17:18:39 by okapshai         ###   ########.fr       */
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
