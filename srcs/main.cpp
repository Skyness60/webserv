#include "ServerManager.hpp"

int	main(int ac, char **av)
{
	try {
		if (ac != 2)
			throw std::invalid_argument("Usage: ./program <filename>");
		ServerManager server(av[1]);
	} catch (const std::exception &e) {
		std::cerr << BOLD_RED "Error: " << e.what() << RESET << std::endl;
		return 1;
	}
}