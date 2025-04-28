#include "CGIManager.hpp"

CGIManager::CGIManager(Config &config, int serverIndex) : _config(config), _serverIndex(serverIndex)
{
	std::string path;
	std::string extension;
	std::string root;
	const std::vector<std::string> &locations = _config.getLocationName(serverIndex);

	for (std::vector<std::string>::const_iterator it = locations.begin(); it != locations.end(); ++it)
	{
		path = _config.getLocationValue(serverIndex, *it, "cgi_path");
		extension = _config.getLocationValue(serverIndex, *it, "cgi_extension");
		root = _config.getLocationValue(serverIndex, *it, "cgi_root");
	
		if (!path.empty() and !extension.empty() and !root.empty())
		{
			_path = path;
			_extension = extension;
			_root = root;
			_locationName = *it;
			break;
		}
		else
		{
			path = "";
			extension = "";	
			root = "";
		}
	}
}

CGIManager::CGIManager(const CGIManager &copy) : _config(copy._config), _serverIndex(copy._serverIndex)
{
	_extension = copy._extension;
	_path = copy._path;
	_root = copy._root;
	_locationName = copy._locationName;
}

CGIManager &CGIManager::operator=(const CGIManager &copy) {
	if (this != &copy) {
		_config = copy._config;
		_serverIndex = copy._serverIndex;
		_extension = copy._extension;
		_path = copy._path;
		_root = copy._root;
		_locationName = copy._locationName;
	}
	return *this;
}

CGIManager::~CGIManager() {}

void CGIManager::executeCGI(int client_fd, const std::string &method) {
	(void)client_fd;
	(void)method;
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("pipe");
		return;
	}
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		return;
	}
	if (pid == 0) {
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
		execve(_path.c_str(), NULL, NULL);
		perror("execl");
		exit(EXIT_FAILURE);
	}
	else{
		close(pipe_fd[1]);
		char buffer[1024];
		ssize_t bytesRead;
		while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytesRead] = '\0';
			std::cout << buffer;
		}
		if (bytesRead == -1) {
			perror("read");
		}
		close(pipe_fd[0]);
		int status;
		waitpid(pid, &status, 0);
		send(client_fd, buffer, bytesRead, MSG_NOSIGNAL);
	}
}

std::string CGIManager::getPath() const {
	return _path;
}

std::string CGIManager::getExtension() const {
	return _extension;
}

std::string CGIManager::getRoot() const {
	return _root;
}

std::string CGIManager::getLocationName() const {
	return _locationName;
}

