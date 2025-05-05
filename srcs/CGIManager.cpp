#include "CGIManager.hpp"

CGIManager::CGIManager(Config &config, int serverIndex, ClientRequest &request) : _config(config), _serverIndex(serverIndex), _request(request)
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
	initEnv(this->_env);
}

CGIManager::CGIManager(const CGIManager &copy) : _config(copy._config), _serverIndex(copy._serverIndex), _request(copy._request)
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

void CGIManager::initEnv(std::map<std::string, std::string> &env) {
	env["GATEWAY_INTERFACE"] = "CGI/1.1";
	env["SERVER_SOFTWARE"] = "webserv/1.0";
	env["SERVER_NAME"] = _config.getConfigValue(_serverIndex, "server_name");
	env["SERVER_PORT"] = _config.getConfigValue(_serverIndex, "port");
	env["REQUEST_METHOD"] = _request.getMethod();
	env["SCRIPT_FILENAME"] = _path;
	env["PATH_INFO"] = _request.getPath();
	env["QUERY_STRING"] = _request.getResourcePath();
	env["CONTENT_TYPE"] = _request.getContentType();
	env["CONTENT_LENGTH"] = std::to_string(_request.getBody().size());
}

char **CGIManager::createEnvArray() {
	char **envArray = new char*[_env.size() + 1];
	int i = 0;
	for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); ++it) {
		std::string envVar = it->first + "=" + it->second;
		envArray[i] = new char[envVar.size() + 1];
		strcpy(envArray[i], envVar.c_str());
		i++;
	}
	envArray[i] = NULL;
	return envArray;
}

static std::string getScriptName(std::string path){
	size_t pos = path.find_first_of('?');
	if (pos != std::string::npos) {
		path = path.substr(0, pos);
		return path;
	}
	pos = path.find_last_of('/');
	if (pos != std::string::npos) {
		return path.substr(pos + 1);
	}
	return path;
}

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
		char **env = createEnvArray();
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
		std::string scriptPath = _root + "/" + getScriptName(_request.getPath());
		char *args[3];
		args[0] = new char[this->_path.size() + 1];
		strcpy(args[0], this->_path.c_str());
		args[1] = new char[scriptPath.size() + 1];
		strcpy(args[1], scriptPath.c_str());
		args[2] = NULL;
		//std::cout << "Executing CGI script: " << args[1] << std::endl;
		//std::cout << "CGI path: " << this->_path << std::endl;
		execve(this->_path.c_str(), args, env);
		perror("execl");
		delete [] args[0];
		delete [] args[1];
		delete [] env;
		exit(EXIT_FAILURE);
	}
	else{
		close(pipe_fd[1]);
		char buffer[1024];
		ssize_t bytesRead;
		std::string cgiOutput;
		while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
			buffer[bytesRead] = '\0';
			cgiOutput += buffer;
		}
		if (bytesRead == -1) {
			perror("read");
		}
		close(pipe_fd[0]);
		int status;
		waitpid(pid, &status, 0);
		std::cout << "CGI result: " << cgiOutput << std::endl;
		send(client_fd, cgiOutput.c_str(), cgiOutput.size(), MSG_NOSIGNAL);
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

