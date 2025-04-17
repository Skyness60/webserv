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

void CGIManager::executeCGI(int client_fd, const std::string &method, const std::string &queryString) {
	(void)client_fd;
	(void)method;
	(void)queryString;
		
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

