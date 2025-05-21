#include "ServerManager.hpp"
#include "ClientRequest.hpp"
#include "DdosProtection.hpp"

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
		signal(SIGPIPE, SIG_IGN);
		ServerManager server(av[1]);
		server.loadConfig();
		server.startServer();
	} catch (const std::exception & e) {
		std::cerr << BOLD_RED << e.what() << RESET << std::endl;
		return 1;
	}
	return 0;
}