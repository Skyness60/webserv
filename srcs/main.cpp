#include "ServerManager.hpp"
#include "ClientRequest.hpp"

static bool checkConf(char *av)
{
	std::string conf(av);
	std::string ext = ".conf";
	if (conf.length() <= ext.length())
		return false;
	return conf.compare(conf.length() - ext.length(), ext.length(), ext) == 0;
}

int	main(int ac, char **av)
{
	try {
		if (ac != 2)
			throw std::invalid_argument("[webserv_main] ERROR Usage: ./webserv <config.conf>");
		if (!checkConf(av[1]))
			throw std::invalid_argument("[webserv_main] ERROR Invalid config file extension");
		ServerManager server(av[1]);

		// L'affichage des valeurs de configuration 
		std::vector<std::map<std::string, std::string> > configValues = server.getConfigValues();
		std::vector<std::map<std::string, std::map<std::string, std::string> > > locationValues = server.getLocationValues();
		for (size_t i = 0; i < configValues.size(); ++i) {
			std::cout << "Server " << i << ":" << std::endl;
			std::cout << "[" << std::endl;
			PRINT_CONFIG(configValues[i]);
			std::cout << "]" << std::endl;
		}
		for (size_t i = 0; i < locationValues.size(); ++i) {
			std::cout << "Location " << i << ":" << std::endl;
			std::cout << "[" << std::endl;
			for (std::map<std::string, std::map<std::string, std::string> >::const_iterator it = locationValues[i].begin(); it != locationValues[i].end(); ++it) {
				std::cout << "\t[\"" << it->first << "\", {";
				for (std::map<std::string, std::string>::const_iterator innerIt = it->second.begin(); innerIt != it->second.end(); ++innerIt) {
					if (innerIt != it->second.begin())
						std::cout << ", ";
					std::cout << "\"" << innerIt->first << "\": \"" << innerIt->second << "\"";
				}
				std::cout << "}]" << std::endl;
			}
			std::cout << "]" << std::endl;
		}
		server.loadConfig();
		server.startServer();

	} catch (const std::exception & e) {
		std::cerr << BOLD_RED << e.what() << RESET << std::endl;
		return 1;
	}
	return 0;
}