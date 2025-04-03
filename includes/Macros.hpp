#define PRINT_CONFIG_KEY(config, key) \
do { \
	for (std::map<std::string, std::string>::const_iterator it = (config).begin(); it != (config).end(); ++it) { \
		if (it->first == (key)) { \
			std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl; \
		} \
	} \
} while (0)

#define PRINT_CONFIG(config) \
do { \
	for (std::map<std::string, std::string>::const_iterator it = (config).begin(); it != (config).end(); ++it) { \
		std::cout << "\t[\"" << it->first << "\", \"" << it->second << "\"]" << std::endl; \
	} \
} while (0)
