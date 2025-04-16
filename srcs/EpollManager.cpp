#include "EpollManager.hpp"

int EpollManager::setupEpollInstance(const std::vector<int> &server_fds) {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return -1;
    }

    for (int server_fd : server_fds) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = server_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
            perror("epoll_ctl");
            close(epoll_fd);
            return -1;
        }
    }
    return epoll_fd;
}

void EpollManager::eventLoop(int epoll_fd, const std::vector<int> &server_fds, std::function<void(int)> handleNewConnection, std::function<void(int)> handleClientRequest) {
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            int event_fd = events[i].data.fd;
            if (std::find(server_fds.begin(), server_fds.end(), event_fd) != server_fds.end()) {
                handleNewConnection(event_fd);
            } else {
                handleClientRequest(event_fd);
            }
        }
    }
}
