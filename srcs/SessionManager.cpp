#include "SessionManager.hpp"

SessionManager::SessionManager(int timeout) : _sessionTimeout(timeout) {
    std::srand(std::time(nullptr));
}

std::string SessionManager::generateSessionId() {
    std::string sessionId;
    for (int i = 0; i < 16; ++i) {
        sessionId += "0123456789abcdef"[std::rand() % 16];
    }
    return sessionId;
}

std::string SessionManager::createSession() {
    std::string sessionId = generateSessionId();
    _sessions[sessionId]["created_at"] = std::to_string(std::time(nullptr));
    return sessionId;
}

bool SessionManager::isValidSession(const std::string &sessionId) {
    if (_sessions.find(sessionId) == _sessions.end()) {
        return false;
    }
    time_t createdAt = std::stol(_sessions[sessionId]["created_at"]);
    if (std::time(nullptr) - createdAt > _sessionTimeout) {
        _sessions.erase(sessionId);
        return false;
    }
    return true;
}

void SessionManager::setSessionData(const std::string &sessionId, const std::string &key, const std::string &value) {
    if (isValidSession(sessionId)) {
        _sessions[sessionId][key] = value;
    }
}

std::string SessionManager::getSessionData(const std::string &sessionId, const std::string &key) {
    if (isValidSession(sessionId) && _sessions[sessionId].find(key) != _sessions[sessionId].end()) {
        return _sessions[sessionId][key];
    }
    return "";
}

void SessionManager::cleanupExpiredSessions() {
    for (auto it = _sessions.begin(); it != _sessions.end();) {
        if (!isValidSession(it->first)) {
            it = _sessions.erase(it);
        } else {
            ++it;
        }
    }
}
