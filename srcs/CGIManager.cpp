#include "CGIManager.hpp"
#include <sys/select.h>    
#include <signal.h>        

CGIManager::CGIManager(Config &config, int serverIndex, ClientRequest &request, int client_fd, std::string locationName)
    :  _config(config), _serverIndex(serverIndex), _request(request)
{
	_locationName = locationName;
	size_t lastSlash = _locationName.find_last_of('/');
	if (lastSlash != std::string::npos) {
		_locationName = _locationName.substr(0, lastSlash + 1);
	}
	_client_fd = client_fd;
	_response = new Response(client_fd, request, config, serverIndex);
	_root = _config.getConfigValue(serverIndex, "root");

	std::string reqPath = _request.getPath();
	size_t dot = reqPath.find_last_of('.');
	std::string reqExt = (dot != std::string::npos) ? reqPath.substr(dot + 1) : "";

	std::string scriptPath = _root + "/" + reqPath;
	std::ifstream scriptFile(scriptPath.c_str());
	std::string shebangInterpreter;
	if (scriptFile.is_open()) {
		std::string shebang;
		std::getline(scriptFile, shebang);
		if (shebang.substr(0, 2) == "#!") {
			shebangInterpreter = shebang.substr(2);
			size_t pos = shebangInterpreter.find_first_of(" \t");
			if (pos != std::string::npos) {
				shebangInterpreter = shebangInterpreter.substr(0, pos);
			}
		}
		scriptFile.close();
	}

	if (!shebangInterpreter.empty()) {
		_path = shebangInterpreter;
		_extension = reqExt;
	} else {
		const std::vector<std::string> &locations = _config.getLocationName(serverIndex);
		for (std::vector<std::string>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
			if (_locationName not_eq(*it)) continue;
			std::string cgiExt = _config.getLocationValue(serverIndex, *it, "cgi_extension");
			if (cgiExt.empty()) continue;
			std::istringstream iss(cgiExt);
			std::string extToken;
			while (iss >> extToken) {
				if (extToken == reqExt || extToken == "." + reqExt) {
					_extension = extToken;
					_path      = _config.getLocationValue(serverIndex, *it, "cgi_path");
					std::string cgiRoot = _config.getLocationValue(serverIndex, *it, "cgi_root");
					if (!cgiRoot.empty())
						_root = cgiRoot;
					else
						_root = _config.getConfigValue(serverIndex, "root");

					goto found_cgi;
				}
			}
		}
	}
found_cgi:

	initEnv(this->_env);
}

CGIManager::CGIManager(const CGIManager &copy) : _config(copy._config), _serverIndex(copy._serverIndex), _request(copy._request), _response(copy._response)
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

CGIManager::~CGIManager() {
	delete _response;
}

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
	
	if (headerEnd == std::string::npos) {
		headerEnd = cgiOutput.find("\n\n");
		
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
		finalHeaders += "Content-Type: text/html\r\n"; 
	finalHeaders += "Connection: close\r\n";
	return finalHeaders;
}


void CGIManager::executeCGI(const std::string &method) {
    (void)method;
    const int CGI_TIMEOUT = 10; 

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
		dup2(pipe_out[1], STDERR_FILENO); 
		close(pipe_in[0]);
		close(pipe_out[1]);
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
	else {
		
		close(pipe_in[0]);
		close(pipe_out[1]);

		
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(pipe_out[0], &rfds);
		struct timeval tv;
		tv.tv_sec = CGI_TIMEOUT;
		tv.tv_usec = 0;
		int sel = select(pipe_out[0] + 1, &rfds, NULL, NULL, &tv);
		if (sel == 0) {
			
			kill(pid, SIGKILL);
			close(pipe_out[0]);
			_response->safeSend(500, "Internal Server Error",
                                "CGI script timed out.\n",
                                "text/plain", true);
			return;
		}
		
		char buffer[4096];
		ssize_t bytesRead;
		std::string cgiOutput;
		while ((bytesRead = read(pipe_out[0], buffer, sizeof(buffer))) > 0) {
			cgiOutput.append(buffer, bytesRead);
		}

		if (bytesRead == -1) {
			perror("read");
			std::cerr << "Error reading from CGI output pipe" << std::endl;
			close(pipe_out[0]);
			close(_client_fd);
			_response->handleNotFound();
			return;
		}
		close(pipe_out[0]);

		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
			std::cerr << "CGI script exited with status: " << WEXITSTATUS(status) << std::endl;
			_response->safeSend(500, "Internal Server Error",
                                "CGI script error.\n",
                                "text/plain", true);
			return;
		}

		std::pair<std::string, std::string> parsed = parseCGIResponse(cgiOutput);
		std::string response = "HTTP/1.1 200 OK\r\n";
		response += buildFinalHeaders(parsed.first, parsed.second.size());
		response += "\r\n" + parsed.second;
		LOG_INFO("CGIManager sending HTTP code: 200");
        if (send(_client_fd, response.c_str(), response.length(), 0) <= 0) {
            perror("send CGI response");
        }
		shutdown(_client_fd, SHUT_RDWR);
		close(_client_fd);
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

