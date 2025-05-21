#pragma once

#include <string>
#include <map>
#include <ctime>

struct Session {
    std::map<std::string, std::string> data;
    time_t                             lastAccess;
    int                                timeout;
};

class SessionManager {
public:
    SessionManager(int timeout = 3600);

    std::string createSession();
    bool isValidSession(const std::string &sid);
    bool sessionExists(const std::string &sid) const;

    void setSessionData(const std::string &sid,
                        const std::string &key,
                        const std::string &value);
    std::string getSessionData(const std::string &sid,
                               const std::string &key);

    void deleteSession(const std::string &sid);
    void cleanupExpiredSessions();
    void addSession(const std::string &sid);

private:
    std::string generateSessionId();
    int _sessionTimeout;
    std::map<std::string, Session> _sessions;
};
