#pragma once

#include <unistd.h>				// execve, dup, dup2, pipe, fork, close, read, write, chdir, access, stat, open, fcntl
#include <sys/types.h>			// pid_t, ssize_t
#include <sys/stat.h>			// stat
#include <sys/socket.h>			// socket, accept, listen, send, recv, setsockopt, getpeername, getsockname
#include <netinet/in.h>			// htons, htonl, ntohs, ntohl, sockaddr_in
#include <arpa/inet.h>			// inet_addr, inet_ntoa
#include <netdb.h>				// getaddrinfo, freeaddrinfo, gai_strerror, getprotobyname
#include <sys/wait.h>			// waitpid
#include <sys/select.h>			// select, FD_SET, FD_ZERO, FD_ISSET
#include <poll.h>				// poll
#include <sys/epoll.h>			// epoll_create, epoll_ctl, epoll_wait
#include <sys/time.h>			// struct timeval (pour select/poll)
#include <dirent.h>				// opendir, readdir, closedir
#include <signal.h>				// kill, signal
#include <errno.h>				// errno, strerror
#include <fcntl.h>				// open, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_APPEND, O_TRUNC
#include <iostream>				// std::cout, std::cerr
#include <string>				// std::string
#include <map>					// std::map
#include <vector>				// std::vector
#include <fstream>				// std::ifstream, std::ofstream
#include "ColorHandling.hpp"	// Color handling
#include "Config.hpp"			// Config class
#include <sstream>				// std::ostringstream
#include <fstream>				// std::ifstream
#include "Macros.hpp"			// Macros
#include "Macros.hpp"			// Macros
//#include <sys/event.h>          // for MAC
