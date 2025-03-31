#include "ServerManager.hpp"

ServerManager::ServerManager(std::string filename) : _filename(filename)
{
	loadConfig();
}
ServerManager::ServerManager(const ServerManager &copy) : _filename(copy._filename)
{
	loadConfig();
}
ServerManager &ServerManager::operator=(const ServerManager &copy)
{
	if (this != &copy)
	{
		_filename = copy._filename;
		loadConfig();
	}
	return *this;
}
ServerManager::~ServerManager()
{
	// Destructor implementation
}
void ServerManager::loadConfig()
{
	std::ifstream configFile(_filename.c_str());
	if (!configFile.is_open())
		throw std::runtime_error("Could not open config file");
	std::string line;
	std::cout << "Loading config file: " << _filename << std::endl;
	while (std::getline(configFile, line))
		std::cout << line << std::endl;
	configFile.close();
}