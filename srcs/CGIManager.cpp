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
	
		if (!extension.empty() and !root.empty())
		{
			_path = path;
			_extension = extension;
			_root = root;
			_locationName = *it;
			break;
		}
		else if (!extension.empty())
		{
			_path = path;
			_extension = extension;
			_root = _config.getLocationValue(serverIndex, *it, "root");
			if (_root.empty())
				_root = _config.getConfigValue(serverIndex, "root");
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

std::pair<std::string, std::string> CGIManager::parseCGIResponse(const std::string &cgiOutput)
{
	size_t headerEnd = cgiOutput.find("\r\n\r\n");
	//size_t skip = 4;
	if (headerEnd == std::string::npos) {
		headerEnd = cgiOutput.find("\n\n");
		//skip = 2;
	}
	if (headerEnd == std::string::npos) {
		return std::make_pair("", cgiOutput);
	}
	std::string header = cgiOutput.substr(0, headerEnd);
	std::string body;
	if (cgiOutput[headerEnd] == '\r') {
		body = cgiOutput.substr(headerEnd + 4);
	} else {
		body = cgiOutput.substr(headerEnd + 2);
	}
	return std::make_pair(header, body);
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

static std::string buildFinalHeaders(const std::string &cgiHeaders, size_t bodySize) {
	std::istringstream stream(cgiHeaders);
	std::string line;
	std::string finalHeaders;
	bool hasContentLength = false;
	bool hasContentType = false;

	while (std::getline(stream, line)) {
		if (line.find("Content-Length:") != std::string::npos)
			hasContentLength = true;
		if (line.find("Content-Type:") != std::string::npos)
			hasContentType = true;
		finalHeaders += line + "\r\n";
	}
	if (!hasContentLength)
		finalHeaders += "Content-Length: " + std::to_string(bodySize) + "\r\n";
	if (!hasContentType)
		finalHeaders += "Content-Type: text/html\r\n"; // ou text/plain par défaut
	finalHeaders += "Connection: close\r\n";
	return finalHeaders;
}


void CGIManager::executeCGI(int client_fd, const std::string &method) {
	(void)client_fd;
	(void)method;
	int pipe_in[2];
	int pipe_out[2];
	if (pipe(pipe_in) == -1 || pipe(pipe_out) == -1) {
		perror("pipe");
		return;
	}
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		return;
	}
	if (pid == 0) {
		std::string scriptPath = _root + "/" + getScriptName(_request.getPath());
		std::ifstream scriptFile(scriptPath.c_str());
		if (!scriptFile.is_open()) {
			std::cerr << "Error: Unable to open script file: " << scriptPath << std::endl;
			exit(EXIT_FAILURE);
		}
		std::string shebang;
		std::getline(scriptFile, shebang);
		if (shebang.substr(0, 2) != "#!" and this->_path.empty()) {
			std::cerr << "Error: Invalid CGI script: " << scriptPath << std::endl;
			exit(EXIT_FAILURE);
		}
		else if (shebang.substr(0, 2) == "#!" and this->_path.empty()) {
			std::string interpreter = shebang.substr(2);
			size_t pos = interpreter.find_first_of(" \t");
			if (pos != std::string::npos) {
				interpreter = interpreter.substr(0, pos);
			}
			this->_path = interpreter;
		}
		scriptFile.close();
		if (this->_path.empty()) {
			std::cerr << "Error: No interpreter found for script: " << scriptPath << std::endl;
			exit(EXIT_FAILURE);
		}
		std::cout << "Interpreter path: " << this->_path << std::endl;
		struct stat pathStat;
		if (stat(this->_path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode)) {
			std::cerr << "Error: CGI interpreter path is a directory: " << this->_path << std::endl;
			exit(EXIT_FAILURE);
		}
		if (access(this->_path.c_str(), F_OK) == -1 || access(this->_path.c_str(), X_OK) == -1) {
			std::cerr << "Error: CGI interpreter not found or not executable: " << this->_path << std::endl;
			exit(EXIT_FAILURE);
		}
		char **env = createEnvArray();
		dup2(pipe_in[0], STDIN_FILENO);
		dup2(pipe_out[1], STDOUT_FILENO);
		close(pipe_in[0]);
		close(pipe_out[1]);
		std::cout << "Script path: " << scriptPath << std::endl;
		char *args[3];
		args[0] = new char[this->_path.size() + 1];
		strcpy(args[0], this->_path.c_str());
		args[1] = new char[scriptPath.size() + 1];
		strcpy(args[1], scriptPath.c_str());
		args[2] = NULL;
		execve(this->_path.c_str(), args, env);
		perror("execl");
		delete [] args[0];
		delete [] args[1];
		delete [] env;
		exit(EXIT_FAILURE);
	}
	else{
		close(pipe_in[0]);
		close(pipe_out[1]);
		const std::string &body = _request.getBody();
		write(pipe_in[1], body.c_str(), body.size());
		close(pipe_in[1]);
		char buffer[4096];
		ssize_t bytesRead;
		std::string cgiOutput;
		while ((bytesRead = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
			cgiOutput.append(buffer, bytesRead);
		close(pipe_out[0]);
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			std::cerr << "CGI script exited with status: " << WEXITSTATUS(status) << std::endl;
			std::string errorMsg = "CGI script failed with status: " + std::to_string(WEXITSTATUS(status));
			send(client_fd, errorMsg.c_str(), errorMsg.length(), 0);
			return;
		}
		std::cout << "----- CGI OUTPUT BEGIN -----" << std::endl;
		std::cout << cgiOutput << std::endl;
		std::cout << "----- CGI OUTPUT END   -----" << std::endl;
		std::pair<std::string, std::string> parsed = parseCGIResponse(cgiOutput);
		std::string response = "HTTP/1.1 200 OK\r\n";
		response += buildFinalHeaders(parsed.first, parsed.second.size());
		response += "\r\n";
		response += parsed.second;
		send(client_fd, response.c_str(), response.length(), 0);
		
		// Properly close the connection after sending the response
		if (shutdown(client_fd, SHUT_RDWR) < 0) {
			std::cerr << "Shutdown error on client_fd " << client_fd << ": " << strerror(errno) << std::endl;
		}
		close(client_fd);
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

