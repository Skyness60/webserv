#include "Config.hpp"

static std::string trimString(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}


Config::Config(std::string filename) : _filename(filename)
{
    loadConfig();
}

Config::Config(const Config &copy) : _filename(copy._filename), _configValues(copy._configValues)
{
}

Config &Config::operator=(const Config &copy)
{
    if (this != &copy)
    {
        _filename = copy._filename;
        _configValues = copy._configValues;
    }
    return *this;
}

Config::~Config()
{

}
void Config::loadConfig()
{
	int lineCount = 0;
	int charCountAll = 0;
	std::ifstream configFile(_filename.c_str());
	std::string line;
	if (!configFile.is_open())
		throw std::runtime_error("[webserv_parser] ERROR \"" + _filename + "\" failed (No such file or directory)");

	line = trimString(line);
	int charCount = 0;

	std::map<std::string, std::string> currentServerConfig;
	bool inServerBlock = false;

	while (std::getline(configFile, line))
	{
		lineCount++;
		line = trimString(line);

		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		charCount = 0;
		for (std::string::const_iterator it = line.begin(); it != line.end(); ++it)
			charCount++;
		charCountAll += charCount;
		if (line == "server {")
		{
			if (inServerBlock)
			{
				throw std::runtime_error("Nested server blocks are not allowed");
			}
			inServerBlock = true;
			currentServerConfig.clear();
			continue;
		}

		if (line[0] == '}' && line.size() == 1)
		{
			if (!inServerBlock)
			{
				std::stringstream errorMsg;
				errorMsg << "[webserv_parser] ERROR Failed to parse config \"" << _filename
						 << "\": char " << charCountAll << " (line:" << lineCount << ", col:" << charCount << "): ";
				throw std::runtime_error(errorMsg.str());
			}
			inServerBlock = false;
			_configValues.push_back(currentServerConfig);
			continue;
		}

		size_t semicolonPos = line.find(';');
		if (semicolonPos != std::string::npos)
		{
			size_t spacePos = line.find_first_of(" \t");

			if (spacePos != std::string::npos && spacePos < semicolonPos)
			{
				std::string key = trimString(line.substr(0, spacePos));
				std::string value = trimString(line.substr(spacePos + 1, semicolonPos - spacePos - 1));

				if (inServerBlock)
				{
					currentServerConfig[key] = value;
				}
				else
				{
					// Handle global configs if necessary, or ignore them
				}
			}
			else
			{
				// Handle keys without values if necessary, or ignore them
			}
		}
	}
	if (inServerBlock)
	{
		std::stringstream errorMsg;
		errorMsg << "[webserv_parser] ERROR Failed to parse config \"" << _filename
		<< "\": char " << charCountAll << " (line:" << lineCount << ", col:" << charCount << ")";
		throw std::runtime_error(errorMsg.str());
	}
	configFile.close();
}

std::string Config::getConfigValue(int serverIndex, std::string key)
{
    if (serverIndex < 0 || serverIndex >= static_cast<int>(_configValues.size()))
        return "";

    std::map<std::string, std::string>::iterator it = _configValues.at(serverIndex).find(key);
    if (it == _configValues.at(serverIndex).end())
        return "";
    return it->second;
}

std::vector<std::map<std::string, std::string> > Config::getConfigValues()
{
    return _configValues;
}