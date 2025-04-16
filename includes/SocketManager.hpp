#pragma once

#include "Includes.hpp"

class SocketManager {
public:
    int createAndBindSocket(int serverIndex, const std::string &listenValue);
    struct sockaddr_in getServerAddress(const std::string &listenValue);
};
