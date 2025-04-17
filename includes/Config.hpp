#pragma once

#include "Includes.hpp"

class Config {
	private:
		std::string _filename;
		std::vector<std::map<std::string, std::string> > _configValues;
		std::vector<std::map<std::string, std::map<std::string, std::string> > > _locationValues;
		
		void throwError(const std::string &errorMsg, int lineCount, int charCountAll, int charCountLine);
		void parseConfigFile();
		void parseServerBlock(std::ifstream &configFile, int &lineCount, int &charCountAll, int charCountLine);
		void handleServerBlockStart(bool &inServerBlock, std::map<std::string, std::string> &currentServerConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, int lineCount, int charCountAl, int charCountLine);
		void handleBlockEnd(bool &inLocationBlock, bool &inServerBlock, std::map<std::string, std::string> &currentLocationConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, std::map<std::string, std::string> &currentServerConfig, std::string &currentLocationPath);
		void processLine(const std::string &line, bool inLocationBlock, bool inServerBlock, std::map<std::string, std::string> &currentLocationConfig, std::map<std::string, std::string> &currentServerConfig, int lineCount, int charCountAll, int charCountLine);
		void handleServerBlockStart(bool &inServerBlock, std::map<std::string, std::string> &currentServerConfig, std::map<std::string, std::map<std::string, std::string> > &currentLocationMap, int lineCount, int charCountAll);
		void handleLocationBlockStart(const std::string &line, bool &inLocationBlock, std::string &currentLocationPath, std::map<std::string, std::string> &currentLocationConfig, int lineCount, int charCountAll, int charCountLine);
		public:
		Config();
		Config(std::string filename);
		Config(const Config &copy);
		Config &operator=(const Config &copy);
		~Config();
		void loadConfig();
		std::string getConfigValue(int serverIndex, std::string key);
		std::vector<std::map<std::string, std::string> > getConfigValues();
		std::vector<std::map<std::string, std::map<std::string, std::string> > > getLocationValues();
		std::string getLocationValue(int serverIndex, std::string locationKey, std::string valueKey);
		std::vector<std::string> getLocationName(int index);
		int getLocationCount(int serverIndex);
};
