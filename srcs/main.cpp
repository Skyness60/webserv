#include "ServerManager.hpp"

int	main(int ac, char **av)
{
	try {
		if (ac != 2)
			throw std::invalid_argument("Usage: ./program <filename>");
		ServerManager server(av[1]);

		// L'affichage des valeurs de configuration 
        // std::vector<std::map<std::string, std::string> > configValues = server.getConfigValues();
        // for (size_t i = 0; i < configValues.size(); ++i) {
        //     std::cout << "Server " << i << ":" << std::endl;
        //     for (std::map<std::string, std::string>::const_iterator it = configValues[i].begin(); it != configValues[i].end(); ++it) {
        //         std::cout << "  " << it->first << ": " << it->second << std::endl;
        //     }
        // }
		server.loadConfig();
		// server.startServer();
	} catch (const std::exception &e) {
		std::cerr << BOLD_RED "Error: " << e.what() << RESET << std::endl;
		return 1;
	}
	return 0;
}