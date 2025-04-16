#pragma once

#include "Includes.hpp"
#include <functional> // Ensure this is included for std::function

class EpollManager {
public:
    int setupEpollInstance(const std::vector<int> &server_fds);
    void eventLoop(int epoll_fd, const std::vector<int> &server_fds, std::function<void(int)> handleNewConnection, std::function<void(int)> handleClientRequest);
};
