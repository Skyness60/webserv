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

Config::~Config()
{

}

/**
 * @brief Charge les paramètres de configuration à partir d'un fichier.
 *
 * Cette fonction analyse un fichier de configuration, en extrayant les configurations de serveur.
 * Elle gère les blocs de serveur, les paires clé-valeur et les conditions d'erreur telles que
 * les blocs de serveur imbriqués ou les lignes mal formées.
 *
 * La configuration est stockée dans la variable membre `_configValues`, qui
 * est un vecteur de maps, où chaque map représente la configuration d'un
 * serveur unique. Chaque vecteur est indexé par l'ordre dans lequel les blocs de serveur apparaissent
 *
 * @throws std::runtime_error Si le fichier de configuration ne peut pas être ouvert,
 * si des blocs de serveur imbriqués sont trouvés, ou si
 * le fichier contient des erreurs de syntaxe.
 *
 * @note Cette fonction suppose que le fichier de configuration utilise une syntaxe similaire à :
 * server {
 * clé valeur;
 * ...
 * }
 * Les lignes commençant par '#' sont traitées comme des commentaires et ignorées.
 * Les espaces blancs sont supprimés du début et de la fin des lignes, des clés et des valeurs.
 */

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
                    /**
                     * @brief Gère les configurations globales si nécessaire, ou les ignore.
                     *
                     * Ce bloc est exécuté lorsque la ligne contient une paire clé-valeur,
                     * mais que nous ne sommes pas dans un bloc de serveur.
                     * Actuellement, il n'y a pas de gestion pour ces configurations globales.
                     */
				}
			}
			else
			{
               /**
                 * @brief Gère les clés sans valeurs si nécessaire, ou les ignore.
                 *
                 * Ce bloc est exécuté lorsque la ligne contient un point-virgule,
                 * mais qu'aucun espace n'est trouvé avant.
                 * Actuellement, il n'y a pas de gestion pour ces clés sans valeurs.
                 */
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