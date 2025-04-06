#include "Config.hpp"

Config::Config() {}

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

Config::~Config() {}

void Config::loadConfig()
{
    std::ifstream configFile(_filename.c_str());
    if (!configFile.is_open())
        throw std::runtime_error("[webserv_parser] ERROR Failed to open file \"" + _filename + "\"");

    std::map<std::string, std::string> currentServerConfig;
    std::map<std::string, std::map<std::string, std::string> > currentLocationMap;
    std::map<std::string, std::string> currentLocationConfig;
    bool inServerBlock = false;
    bool inLocationBlock = false;
	std::string line;
    std::string currentLocationPath;

    int lineCount = 0;
    int charCountAll = 0;
	int charCountLine = 0;
    while (std::getline(configFile, line))
    {
		charCountLine = static_cast<int>(line.length());
        lineCount++;
        line = trimString(line);

        if (line.empty() || line[0] == '#')
            continue;

        charCountAll += static_cast<int>(line.length());

        if (line == "server {")
        {
            handleServerBlockStart(inServerBlock, currentServerConfig, currentLocationMap, lineCount, charCountAll, charCountLine);
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
            handleLocationBlockStart(line, inLocationBlock, currentLocationPath, currentLocationConfig, lineCount, charCountAll, charCountLine);
            continue;
        }

        processLine(line, inLocationBlock, inServerBlock, currentLocationConfig, currentServerConfig, lineCount, charCountAll, charCountLine);
    }

    if (inServerBlock || inLocationBlock)
        throwError("Failed to parse config", lineCount, charCountAll, charCountLine);

    configFile.close();
}

void Config::handleServerBlockStart(bool &inServerBlock, std::map<std::string, std::string> &currentServerConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, int lineCount, int charCountAll, int charCountLine)
{
    if (inServerBlock)
    {
        throwError("Nested server block", lineCount, charCountAll, charCountLine);
    }
    currentServerConfig.clear();
    currentLocationMap.clear();
}

void Config::handleBlockEnd(bool &inLocationBlock, bool &inServerBlock, std::map<std::string, std::string> &currentLocationConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, std::map<std::string, std::string> &currentServerConfig, std::string &currentLocationPath)
{
    if (inLocationBlock)
    {
        currentLocationMap[currentLocationPath] = currentLocationConfig;
        currentLocationConfig.clear();
        inLocationBlock = false;
    }
    else if (inServerBlock)
    {
        _configValues.push_back(currentServerConfig);
        _locationValues.push_back(currentLocationMap);
        inServerBlock = false;
    }
}

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

void Config::throwError(const std::string &errorMsg, int lineCount, int charCountAll, int charCountLine)
{
    std::stringstream error;
    error << "[webserv_parser] ERROR " << errorMsg
          << " in file \"" << _filename << "\" at char " << charCountAll
          << " (line " << lineCount << ", column " << charCountLine << ")";
    throw std::runtime_error(error.str());
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

std::vector<std::map<std::string, std::map<std::string, std::string> > > Config::getLocationValues()
{
    return _locationValues;
}

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
