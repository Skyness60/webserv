#pragma once

#include "Includes.hpp"

class Config {
	private:
		std::string _filename;
		std::vector<std::map<std::string, std::string> > _configValues;
	public:
		Config(std::string filename);
		Config(const Config &copy);
		Config &operator=(const Config &copy);
		~Config();
		void loadConfig();
		std::string getConfigValue(int serverIndex, std::string key);
		std::vector<std::map<std::string, std::string> > getConfigValues();

};