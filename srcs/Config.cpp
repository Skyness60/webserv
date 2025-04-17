#include "Config.hpp"

// Utility function to trim whitespace from a string
static std::string trimString(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

// Utility function to extract values enclosed in quotes
static std::string extractValue(const std::string &value)
{
    if (value[0] == '"' && value[value.size() - 1] == '"')
        return value.substr(1, value.size() - 2);
    return value;
}

// Utility function to check if a file exists
static bool fileExists(const std::string &path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Constructor
Config::Config() {}

// Constructor with filename
Config::Config(std::string filename) : _filename(filename)
{
    loadConfig();
}

// Copy constructor
Config::Config(const Config &copy) : _filename(copy._filename), _configValues(copy._configValues), _locationValues(copy._locationValues) {}

// Assignment operator
Config &Config::operator=(const Config &copy)
{
    if (this != &copy)
    {
        _filename = copy._filename;
        _configValues = copy._configValues;
        _locationValues = copy._locationValues;
    }
    return *this;
}

// Destructor
Config::~Config() {}

// Load configuration from file
void Config::loadConfig()
{
    std::ifstream configFile(_filename.c_str());
    if (!configFile.is_open())
        throw std::runtime_error("[webserv_parser] ERROR: Failed to open file \"" + _filename + "\"");

    std::map<std::string, std::string> currentServerConfig;
    std::map<std::string, std::map<std::string, std::string> > currentLocationMap;
    std::map<std::string, std::string> currentLocationConfig;
    bool inServerBlock = false;
    bool inLocationBlock = false;
    std::string line;
    std::string currentLocationPath;

    int lineCount = 0;
    int charCountAll = 0;

    while (std::getline(configFile, line))
    {
        lineCount++;
        line = trimString(line);

        if (line.empty() || line[0] == '#') // Skip empty lines and comments
            continue;

        charCountAll += static_cast<int>(line.length());

        if (line == "server {")
        {
            handleServerBlockStart(inServerBlock, currentServerConfig, currentLocationMap, lineCount, charCountAll, 0);
            inServerBlock = true;
            continue;
        }

        if (line == "}")
        {
            handleBlockEnd(inLocationBlock, inServerBlock, currentLocationConfig, currentLocationMap, currentServerConfig, currentLocationPath);
            continue;
        }

        if (line.substr(0, 8) == "location")
        {
            handleLocationBlockStart(line, inLocationBlock, currentLocationPath, currentLocationConfig, lineCount, charCountAll, 0);
            continue;
        }

        processLine(line, inLocationBlock, inServerBlock, currentLocationConfig, currentServerConfig, lineCount, charCountAll, 0);
    }

    if (inServerBlock || inLocationBlock)
        throwError("Unclosed block detected", lineCount, charCountAll, 0);

    configFile.close();
}

// Handle the start of a server block
void Config::handleServerBlockStart(bool &inServerBlock, std::map<std::string, std::string> &currentServerConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, int lineCount, int charCountAll, int charCountLine)
{
    if (inServerBlock)
        throwError("Nested server block detected", lineCount, charCountAll, charCountLine);

    currentServerConfig.clear();
    currentLocationMap.clear();
}

// Handle the end of a block
void Config::handleBlockEnd(bool &inLocationBlock, bool &inServerBlock, std::map<std::string, std::string> &currentLocationConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, std::map<std::string, std::string> &currentServerConfig, std::string &currentLocationPath)
{
    if (inLocationBlock)
    {
        currentLocationMap[currentLocationPath] = currentLocationConfig;
        inLocationBlock = false;
    }
    else if (inServerBlock)
    {
        _configValues.push_back(currentServerConfig);
        _locationValues.push_back(currentLocationMap);
        inServerBlock = false;
    }
    else
    {
        throwError("Unexpected closing brace", 0, 0, 0);
    }
}
// Handle the start of a location block
void Config::handleLocationBlockStart(const std::string &line, bool &inLocationBlock, std::string &currentLocationPath, std::map<std::string, std::string> &currentLocationConfig, int lineCount, int charCountAll, int charCountLine)
{
    size_t bracePos = line.find('{');
    if (bracePos != std::string::npos)
    {
        currentLocationPath = trimString(line.substr(0, bracePos));
        inLocationBlock = true;
        currentLocationConfig.clear();
    }
    else
    {
        throwError("Malformed location block (missing {)", lineCount, charCountAll, charCountLine);
    }
}
// Process a line of configuration
// This function processes a line of configuration and extracts key-value pairs
void Config::processLine(const std::string &line, bool inLocationBlock, bool inServerBlock, std::map<std::string, std::string> &currentLocationConfig, std::map<std::string, std::string> &currentServerConfig, int lineCount, int charCountAll, int charCountLine)
{
    size_t semicolonPos = line.find(';');
    if (semicolonPos != std::string::npos)
    {
        size_t spacePos = line.find_first_of(" \t");
        if (spacePos != std::string::npos && spacePos < semicolonPos)
        {
            std::string key = trimString(line.substr(0, spacePos));
            std::string value = trimString(line.substr(spacePos + 1, semicolonPos - spacePos - 1));
            value = extractValue(value);
			if (key == "listen")
			{
				int port = std::atoi(value.c_str());
				if (value.empty())
					throwError("Malformed listen directive", lineCount, charCountAll, charCountLine);
				if (port <= 0 || port > 65535)
					throwError("Invalid port number in listen directive", lineCount, charCountAll, charCountLine);
				size_t colonPos = value.find(':');
				if (colonPos != std::string::npos)
				{
					std::string host = trimString(value.substr(0, colonPos));
					std::string port = trimString(value.substr(colonPos + 1));
					if (host.empty() || port.empty())
						throwError("Malformed listen directive", lineCount, charCountAll, charCountLine);
					currentServerConfig[key] = host + ":" + port;
				}
				else
				{
					if (value.empty())
						throwError("Malformed listen directive", lineCount, charCountAll, charCountLine);
					currentServerConfig[key] = value;
				}
			}
			else if (key == "server_name")
			{
				if (value.empty())
					throwError("Malformed server_name directive", lineCount, charCountAll, charCountLine);
				currentServerConfig[key] = value;
			}
			else if (key == "root" || key == "error_page")
			{
				if (key == "error_page")
				{
					size_t spacePos = value.find(' ');
					if (spacePos != std::string::npos)
					{
						std::string errorCode = trimString(value.substr(0, spacePos));
						std::string errorPage = trimString(value.substr(spacePos + 1));
						if (errorCode.empty() || errorPage.empty())
							throwError("Malformed error_page directive", lineCount, charCountAll, charCountLine);
						currentServerConfig[key] = errorCode + " " + errorPage;
						std::cout << "Error code: " << errorCode << ", Error page: " << errorPage << std::endl;
					}
					else
					{
						if (value.empty())
							throwError("Malformed error_page directive", lineCount, charCountAll, charCountLine);
						currentServerConfig[key] = value;
					}
				}
				else if (value.empty())
					throwError("Malformed root directive", lineCount, charCountAll, charCountLine);
				else if (!fileExists(value))
					throwError("File path does not exist: " + value, lineCount, charCountAll, charCountLine);
				currentServerConfig[key] = value;
			}
			else if (key == "index")
			{
				if (value.empty())
					throwError("Malformed index directive", lineCount, charCountAll, charCountLine);
				currentServerConfig[key] = value;
			}
			else if (key == "error_page")
			{
				if (value.empty())
					throwError("Malformed error_page directive", lineCount, charCountAll, charCountLine);
				size_t spacePos = value.find(' ');
				if (spacePos != std::string::npos)
				{
					std::string errorCode = trimString(value.substr(0, spacePos));
					std::string errorPage = trimString(value.substr(spacePos + 1));
					if (errorCode.empty() || errorPage.empty())
						throwError("Malformed error_page directive", lineCount, charCountAll, charCountLine);
					currentServerConfig[key] = errorCode + " " + errorPage;
				}
				else
				{
					if (value.empty())
						throwError("Malformed error_page directive", lineCount, charCountAll, charCountLine);
					currentServerConfig[key] = value;
				}
			}
			else if (key == "autoindex")
			{
				if (value != "on" && value != "off")
					throwError("Malformed autoindex directive", lineCount, charCountAll, charCountLine);
				currentServerConfig[key] = value;
			}
            if (inLocationBlock)
                currentLocationConfig[key] = value;
            else if (inServerBlock)
                currentServerConfig[key] = value;
        }
        else
        {
            throwError("Malformed directive", lineCount, charCountAll, charCountLine);
        }
    }
}
// Handle errors and throw exceptions
void Config::throwError(const std::string &errorMsg, int lineCount, int charCountAll, int charCountLine)
{
    std::stringstream error;
    error << "[webserv_parser] ERROR " << errorMsg
          << " in file \"" << _filename << "\" at char " << charCountAll
          << " (line " << lineCount << ", column " << charCountLine << ")";
    throw std::runtime_error(error.str());
}
// Get the value of a specific key for a specific server
std::string Config::getConfigValue(int serverIndex, std::string key)
{
    if (serverIndex < 0 || serverIndex >= static_cast<int>(_configValues.size()))
        return "";

    std::map<std::string, std::string>::iterator it = _configValues.at(serverIndex).find(key);
    if (it == _configValues.at(serverIndex).end())
        return "";
    return it->second;
}
// Get the values of all keys for a specific server
std::vector<std::map<std::string, std::string> > Config::getConfigValues()
{
    return _configValues;
}
// Get the values of all keys for all servers
std::vector<std::map<std::string, std::map<std::string, std::string> > > Config::getLocationValues()
{
    return _locationValues;
}
// Get the value of a specific key for a specific location in a server
std::string Config::getLocationValue(int serverIndex, std::string locationKey, std::string valueKey)
{
    if (serverIndex < 0 || serverIndex >= static_cast<int>(_locationValues.size()))
        return "";

    std::map<std::string, std::map<std::string, std::string> >::iterator it = _locationValues.at(serverIndex).find(locationKey);
    if (it == _locationValues.at(serverIndex).end())
        return "";

    std::map<std::string, std::string>::iterator innerIt = it->second.find(valueKey);
    if (innerIt == it->second.end())
        return "";

    return innerIt->second;
}

std::vector<std::string> Config::getLocationName(int index){
	std::vector<std::string> vector;
	std::vector<std::map<std::string, std::map<std::string, std::string> > > values = this->getLocationValues();
	for (std::map<std::string, std::map<std::string, std::string> >::iterator it = values[index].begin(); it != values[index].end(); it++){
		vector.push_back(it->first);
	}
	return vector;
}

int Config::getLocationCount(int serverIndex){
	if (serverIndex < 0 || serverIndex >= static_cast<int>(_locationValues.size()))
		return 0;
	return _locationValues.at(serverIndex).size();
}