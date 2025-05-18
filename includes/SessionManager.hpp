#pragma once

#include <string>
#include <map>
#include <ctime>
#include <cstdlib>

class SessionManager {
private:
    std::map<std::string, std::map<std::string, std::string>> _sessions;
    int _sessionTimeout;

    std::string generateSessionId();

public:
    SessionManager(int timeout = 3600);
    std::string createSession();
    bool isValidSession(const std::string &sessionId);
    void setSessionData(const std::string &sessionId, const std::string &key, const std::string &value);
    std::string getSessionData(const std::string &sessionId, const std::string &key);
    void cleanupExpiredSessions();
};
