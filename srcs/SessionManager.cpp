#include "SessionManager.hpp"
#include <cstdlib>
#include <ctime>

SessionManager::SessionManager(int timeout)
    : _sessionTimeout(timeout) {
    std::srand(std::time(nullptr));
}

std::string SessionManager::generateSessionId() {
    std::string id;
    for (int i = 0; i < 16; ++i)
        id += "0123456789abcdef"[std::rand() % 16];
    return id;
}

std::string SessionManager::createSession() {
    std::string sid = generateSessionId();
    Session s;
    s.data["created_at"] = std::to_string(std::time(nullptr));
    s.lastAccess = std::time(nullptr);
    s.timeout    = _sessionTimeout;
    _sessions[sid] = s;
    return sid;
}

void SessionManager::addSession(const std::string &sid) {
    Session s;
    s.data["created_at"] = std::to_string(std::time(nullptr));
    s.lastAccess = std::time(nullptr);
    s.timeout    = _sessionTimeout;
    _sessions[sid] = s;
}

bool SessionManager::isValidSession(const std::string &sid) {
    auto it = _sessions.find(sid);
    if (it == _sessions.end()) return false;
    time_t now = std::time(nullptr);
    if (now - it->second.lastAccess > it->second.timeout) {
        _sessions.erase(it);
        return false;
    }
    it->second.lastAccess = now;
    return true;
}

bool SessionManager::sessionExists(const std::string &sid) const {
    auto it = _sessions.find(sid);
    if (it == _sessions.end()) return false;
    time_t now = std::time(nullptr);
    return (now - it->second.lastAccess <= it->second.timeout);
}

void SessionManager::setSessionData(const std::string &sid,
                                    const std::string &key,
                                    const std::string &value)
{
    if (isValidSession(sid))
        _sessions[sid].data[key] = value;
}

std::string SessionManager::getSessionData(const std::string &sid,
                                           const std::string &key)
{
    if (isValidSession(sid)) {
        auto &m = _sessions[sid].data;
        auto it = m.find(key);
        if (it != m.end()) return it->second;
    }
    return "";
}

void SessionManager::deleteSession(const std::string &sid) {
    _sessions.erase(sid);
}

void SessionManager::cleanupExpiredSessions() {
    for (auto it = _sessions.begin(); it != _sessions.end();) {
        if (!isValidSession(it->first))
            it = _sessions.erase(it);
        else
            ++it;
    }
}
