/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallis <mwallis@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 06:57:28 by mwallis           #+#    #+#             */
/*   Updated: 2026/05/02 06:57:28 by mwallis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

#include <stdexcept>
#include <set>
#include <cctype>  //isspace
#include <cerrno>  // errno, ERANGE
#include <cstdlib> // strtol, strtoul
#include <climits> // INT_MIN, INT_MAX
#include <limits>  // numeric_limits



ConfigParser::ConfigParser()
: _content(""), _pos(0)
{

}

ConfigParser::~ConfigParser()
{

}

ConfigParser::ConfigParser(const ConfigParser& other)
: _content(other._content), _tokens(other._tokens), _pos(other._pos)
{

}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		this->_content = other._content;
		this->_tokens = other._tokens;
		this->_pos = other._pos;
	}
	return (*this);
}

ConfigParser::ConfigParser(const std::string& content)
: _content(content), _pos(0)
{

}

// Prends la string content et la tokenize
void ConfigParser::tokenize()
{
	// reset de _tokens et _pos
	// au cas ou parseConfig() est appelé deux fois sur le même objet,
	// afin de ne pas rajouter les tokens à la suite des anciens.
	this->_tokens.clear();
	this->_pos = 0;

	size_t			contentSize;
	size_t			i;
	unsigned char	c;

	i = 0;
	contentSize = this->_content.size();
	while (i < contentSize)
	{
		c = static_cast<unsigned char>(this->_content[i]);

		// on ignore ' ', '\f', '\n', '\r', '\t', '\v'
		if (std::isspace(c))
		{
			i++;
			continue;
		}

		// gerer les commentaires :
		if (c == '#')
		{
			while (i < contentSize && this->_content[i] != '\n')
			{
				i++;
			}
			continue;
		}


		if (c == '{' || c == '}' || c == ';')
		{
			// convertir unsigned char en string
			std::string tokenSpecial(1, static_cast<char>(c));
			this->_tokens.push_back(tokenSpecial);
			i++;
			continue;
		}

		std::string	word;

		while (i < contentSize 
			&& !std::isspace(static_cast<unsigned char>(this->_content[i]))
			&& this->_content[i] != '{' 
			&& this->_content[i] != '}' 
			&& this->_content[i] != ';'
			&& this->_content[i] != '#')
		{
			word += this->_content[i];
			i++;
		}

		if (word.empty() == false)
		{
			this->_tokens.push_back(word);
		}
	}
}



// retourne le token actuel
std::string ConfigParser::peek() const
{
	if (this->_pos >= this->_tokens.size())
	{
		return ("");
	}

	return (this->_tokens[this->_pos]);
}

// retourne le token actuel et avance :
std::string ConfigParser::consume()
{
	if (isEnd() == true)
	{
		throw std::runtime_error("Unexpected end of file");
	}

	std::string	currentToken;

	currentToken = this->_tokens[this->_pos];
	this->_pos++;

	return (currentToken);
}

// verifie que le token actuel est ce qui est attendu et avance sans rien renvoyer (pour les données inutile)
void ConfigParser::expect(const std::string& expected)
{
	std::string currentToken;

	currentToken = peek();

	if (currentToken != expected)
	{
		throw std::runtime_error("Unexpected token: expected '" + expected + "', got '" + currentToken + "'");
	}

	consume();
}

bool ConfigParser::isEnd() const
{
	if (this->_pos >= this->_tokens.size())
	{
		return (true);
	}
	else
	{
		return (false);
	}
}


static bool verifyParseStringToSizeT(const std::string& str, size_t& result)
{
	if (str.empty() == true || str[0] == '-' || str[0] == '+')
	{
		return (false);
	}

    errno = 0;
    // errno est une variable globale (plus précisément, thread-local globale dans les versions modernes du C/C++)
    // Elle ne se réinitialise jamais automatiquement
    // Elle n’est pas remise à zéro quand tout va bien
    // Certaines fonctions ne la modifient pas du tout
    // En C++ moderne, chaque thread a sa propre copie

    char			*end;
	unsigned long	tmpUnsignedLong;

    end = NULL;
	tmpUnsignedLong = std::strtoul(str.c_str() ,&end, 10);

    // Vérification des erreurs :
    // 1) ernno == ERANGE -> Overflow / underflow
    // 2) end == literal.c_str()  -> ptr de end pointe au meme endroit que le début de la chaine literal donc aucun caractere lu
    // 3) *end != '\0'            -> il reste des caractères non parsés apres notre nombre
    if (errno == ERANGE || end == str.c_str() || *end != '\0')
    {
        return (false);
    }

	if (tmpUnsignedLong > std::numeric_limits<size_t>::max())
	{
		return (false);
	}

	result = static_cast<size_t>(tmpUnsignedLong);

    return (true);
}

static bool verifyParseStringToInt(const std::string& str, int& result)
{
	if (str.empty() == true || str[0] == '+')
	{
		return (false);
	}

    errno = 0;

    char	*end;
	long	tmpLong;

    end = NULL;
	tmpLong = std::strtol(str.c_str() ,&end, 10);

    if (errno == ERANGE || end == str.c_str() || *end != '\0')
    {
        return (false);
    }
		
	if (tmpLong < INT_MIN || tmpLong > INT_MAX)
	{
		return (false);
	}

	result = static_cast<int>(tmpLong);

    return (true);
}


void ConfigParser::parseMethods(LocationConfig& location)
{
	expect("methods");

	if (location.hasMethods == true)
	{
		throw std::runtime_error("duplicate methods directive");
	}
	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("methods directive requires at least one method");
	}

	while (isEnd() == false && peek() != ";")
	{
		if (peek() == "}" || peek() == "{")
		{
			throw std::runtime_error("missing ';' after the methods directive");
		}

		std::string methodName;

		methodName = consume();
		if (methodName != "GET" && methodName != "POST" && methodName != "DELETE" && methodName != "HEAD")
		{
			throw std::runtime_error("Invalid method: '" + methodName + "'\naccepted methods: GET, POST, DELETE and HEAD");
		}
		// verifie les doublons et insert :
		if (location.methods.insert(methodName).second == false)
		{
			throw std::runtime_error("Duplicate method: '" + methodName + "'");
		}
	}
 
	expect(";");
	location.hasMethods = true;
}

void ConfigParser::parseAutoindex(LocationConfig& location)
{
	expect("autoindex");

	if (location.hasAutoindex == true)
	{
		throw std::runtime_error("duplicate autoindex directive");
	}
	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("autoindex directive requires a value");
	}

	std::string autoIndexValue;

	autoIndexValue = consume();
	if (autoIndexValue == "on")
	{
		location.autoindex = true;
	}
	else if (autoIndexValue == "off")
	{
		location.autoindex = false;
	}
	else
	{
		throw std::runtime_error("autoindex must be 'on' or 'off'");
	}

	expect(";");

	location.hasAutoindex = true;
}

void ConfigParser::parseReturn(LocationConfig& location)
{
	expect("return");

	if (location.hasRedirect == true)
	{
		throw std::runtime_error("duplicate return directive");
	}
	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("return directive requires a status code");
	}

	int			codeInt;
	std::string	codeString;

	codeString = consume();

	if (verifyParseStringToInt(codeString, codeInt) == false)
	{
		throw std::runtime_error("Invalid return code");
	}
	if (codeInt < 300 || codeInt > 399)
	{
		throw std::runtime_error("return code must be between 300 and 399");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("return directive requires a target");
	}

	location.redirectCode = codeInt;
	location.redirectTarget = consume();

	expect(";");

	location.hasRedirect = true;
}

void ConfigParser::parseUpload(LocationConfig& location)
{
	expect("upload");

	if (location.hasUpload == true)
	{
		throw std::runtime_error("duplicate upload directive");
	}
	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("upload directive requires a value");
	}

	std::string uploadValue;

	uploadValue = consume();

	if (uploadValue == "on")
	{
		location.uploadEnabled = true;
	}
	else if (uploadValue == "off")
	{
		location.uploadEnabled = false;
	}
	else
	{
		throw std::runtime_error("upload must be 'on' or 'off'");
	}

	expect(";");

	location.hasUpload = true;
}

void ConfigParser::parseUploadStore(LocationConfig& location)
{
	expect("upload_store");

	if (location.uploadStore.empty() == false)
	{
		throw std::runtime_error("duplicate upload_store directive");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("upload_store requires a path");
	}

	location.uploadStore = consume();

	expect(";");

}

// cgi[".php"] = "/usr/bin/php-cgi";    valide 
// cgi[".py"]  = "/usr/bin/python3";    valide
// cgi[".php"]   = "/usr/bin/php-cgi";  invalide car doublon
// cgi[".phtml"] = "/usr/bin/php-cgi";  valide meme si meme binaire
// cgi["phtml"] = "/usr/bin/php-cgi";   invalide
void ConfigParser::parseCgi(LocationConfig& location)
{
	expect("cgi");

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("cgi directive requires an extension");
	}

	std::string	extension;

	extension = consume();

	if (extension.empty() == true || extension[0] != '.' || extension.size() == 1)
	{
		throw std::runtime_error("cgi extension must start with '.' and contain an extension name");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("cgi directive requires an executable path");
	}

	// find cherche dans les clés (keys), PAS dans les valeurs.
	if (location.cgi.find(extension) != location.cgi.end())
	{
		throw std::runtime_error("cgi handler already defined for extension '" + extension + "'");
	}

	std::string executable;

	executable = consume();

	expect(";");

	location.cgi[extension] = executable;

}







void ConfigParser::parseLocationClientMaxBodySize(LocationConfig& location)
{
	std::string	valueString;
	size_t		valueSizeT;
	

	expect("client_max_body_size");

	if (location.hasClientMaxBodySize == true)
	{
		throw std::runtime_error("duplicate client_max_body_size directive in location block");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("client_max_body_size directive in location block requires a value");
	}

	valueString = consume();

	if (verifyParseStringToSizeT(valueString, valueSizeT) == false)
	{
		throw std::runtime_error("Invalid client_max_body_size in location block");
	}
	// Un size_t est unsigned donc impossible d'etre inferieur a zero
	if (valueSizeT == 0)
	{
		throw std::runtime_error("client_max_body_size value in location block must be greater than zero");
	}

	location.clientMaxBodySize = valueSizeT;
	location.hasClientMaxBodySize = true;

	expect(";");
}



void ConfigParser::parseLocationBody(LocationConfig& location)
{
	while (isEnd() == false && peek() != "}")
	{
		if (peek() == "root")
		{
			parseRoot(location.root);
		}
		else if (peek() == "index")
		{
			parseIndex(location.index);
		}
		else if (peek() == "methods")
		{
			parseMethods(location);
		}
		else if (peek() == "autoindex")
		{
			parseAutoindex(location);
		}
		else if (peek() == "return")
		{
			parseReturn(location);
		}
		else if (peek() == "upload")
		{
			parseUpload(location);
		}
		else if (peek() == "upload_store")
		{
			parseUploadStore(location);
		}
		else if (peek() == "cgi")
		{
			parseCgi(location);
		}
		else if (peek() == "client_max_body_size")
		{
			parseLocationClientMaxBodySize(location);
		}
		else
		{
			throw std::runtime_error("Unknown location directive: '" + peek() + "'");
		}
	}
}


LocationConfig ConfigParser::parseLocation()
{
	LocationConfig	location;

	expect("location");

	if (isEnd() == true || peek() == ";" || peek() == "{" || peek() == "}")
	{
		throw std::runtime_error("location directive requires a path");
	}

	location.path = consume();

	if (location.path.empty() == true || location.path[0] != '/')
	{
		throw std::runtime_error("location path must start with '/' and cannot be empty");
	}


	expect("{");
	parseLocationBody(location);
	expect("}");

	if (location.hasUpload == true && location.uploadEnabled == true && location.uploadStore.empty() == true)
	{
		throw std::runtime_error("upload is enabled but upload_store path is missing");
	}

	if (location.uploadStore.empty() == false && (location.hasUpload == false || location.uploadEnabled == false))
	{
		throw std::runtime_error("upload_store requires 'upload on'");
	}

	return (location);
}





// vérifier si il y'a besoin ou non de faire des vérification avec host !!! Seul endroit non vérifier
// si getaddrinfo ou bind est utilisé, ce qui est probable, aucun besoin de parsing ! ce serait du doublon
// un mauvais format de host échouera au bind/getaddrinfo si invalide

void ConfigParser::parseListen(ServerConfig& server)
{
	expect("listen");

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("listen directive requires a value");
	}

	ListenConfig	tmpListenConfig;
	std::string		valueFromListen;
	std::string		host;
	std::string		portString;
	int				portInt;
	size_t			positionColon;

	valueFromListen = consume();
	positionColon = valueFromListen.find(':');

	if (positionColon == std::string::npos)
	{
		host = "0.0.0.0";
		portString = valueFromListen;
	}
	else
	{
		if (valueFromListen.find(':', positionColon + 1) != std::string::npos)
		{
			throw std::runtime_error("Invalid listen format");
		}

		host = valueFromListen.substr(0, positionColon);
		portString = valueFromListen.substr(positionColon + 1);

		if (host.empty() == true || portString.empty() == true)
		{
			throw std::runtime_error("Invalid listen format\nexpected format : listen 127.0.0.1:8080; or listen 8002;");
		}
	}



	if (verifyParseStringToInt(portString, portInt) == false)
	{
		throw std::runtime_error("Error: Invalid port in listen");
	}
	if (portInt < 1 || portInt > 65535)
	{
		throw std::runtime_error("Port must be between 1 and 65535");
	}

	tmpListenConfig.host = host;
	tmpListenConfig.port = portInt;

	expect(";");


	// verification doublons :
	size_t	i;

	i = 0;
	while (i < server.listens.size())
	{
		if (server.listens[i].host == tmpListenConfig.host 
			&& server.listens[i].port == tmpListenConfig.port)
		{
			throw std::runtime_error("duplicate listen directive");
		}
		i++;
	}

	server.listens.push_back(tmpListenConfig);
}




void ConfigParser::parseServerName(ServerConfig& server)
{
	expect("server_name");

	if (server.serverNames.empty() == false)
	{
		throw std::runtime_error("duplicate server_name directive");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("server_name directive requires at least one name");
	}

	std::set<std::string>	verifyDuplicateServerName;

	while (isEnd() == false && peek() != ";")
	{
		if (peek() == "}" || peek() == "{")
		{
			throw std::runtime_error("missing ';' after the server_name directive\n'server_name localhost {' is invalid");
		}

		std::string	serverName;

		serverName = consume();

		if (verifyDuplicateServerName.insert(serverName).second == false)
		{
			throw std::runtime_error("duplicate server_name value");
		}

		server.serverNames.push_back(serverName);
	}
 
	expect(";");
}




void ConfigParser::parseClientMaxBodySize(ServerConfig& server)
{
	
	std::string	valueString;
	size_t		valueSizeT;
	

	expect("client_max_body_size");

	if (server.hasClientMaxBodySize == true)
	{
		throw std::runtime_error("duplicate client_max_body_size directive");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("client_max_body_size directive requires a value");
	}

	valueString = consume();

	if (verifyParseStringToSizeT(valueString, valueSizeT) == false)
	{
		throw std::runtime_error("Invalid client_max_body_size");
	}
	// Un size_t est unsigned donc impossible d'etre inferieur a zero
	if (valueSizeT == 0)
	{
		throw std::runtime_error("The value of 'client_max_body_size' must be greater than zero");
	}

	server.clientMaxBodySize = valueSizeT;
	server.hasClientMaxBodySize = true;

	expect(";");
}





// Invalide : 
// error_page 404 500 ./errors/error.html;

// Valide :
// error_page 404 ./errors/404.html;
// error_page 500 ./errors/500.html;

void ConfigParser::parseErrorPage(ServerConfig& server)
{
	expect("error_page");

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("error_page directive requires a status code");
	}

	std::string codeString;
	int			codeInt;

	codeString = consume();

	if (verifyParseStringToInt(codeString, codeInt) == false)
	{
		throw std::runtime_error("Invalid error_page status code");
	}

	if (codeInt < 300 || codeInt > 599)
	{
		throw std::runtime_error("error_page status code must be between 300 and 599");
	}

	// Le code existe deja donc find n'a pas retourner end()
	if (server.errorPages.find(codeInt) != server.errorPages.end())
	{
		throw std::runtime_error("duplicate error_page for status code " + codeString);
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("error_page directive requires a path that follows its error code");
	}


	std::string path;

	path = consume();

	expect(";");

	server.errorPages[codeInt] = path;
}


void ConfigParser::parseRoot(std::string& root)
{
	expect("root");

	if (root.empty() == false)
	{
		throw std::runtime_error("duplicate root directive");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("root directive requires a path");
	}

	root = consume();

	expect(";");
}


void ConfigParser::parseIndex(std::string& index)
{
	expect("index");

	if (index.empty() == false)
	{
		throw std::runtime_error("duplicate index directive");
	}

	if (isEnd() == true || peek() == ";" || peek() == "}" || peek() == "{")
	{
		throw std::runtime_error("index directive requires a file");
	}

	index = consume();

	expect(";");
}


void ConfigParser::parseServerBody(ServerConfig& server)
{
	while (isEnd() == false && peek() != "}")
	{
		if (peek() == "listen")
		{
			parseListen(server);
		}
		else if (peek() == "root")
		{
			parseRoot(server.root);
		}
		else if (peek() == "index")
		{
			parseIndex(server.index);
		}
		else if (peek() == "client_max_body_size")
		{
			parseClientMaxBodySize(server);
		}
		else if (peek() == "error_page")
		{
			parseErrorPage(server);
		}
		else if (peek() == "server_name")
		{
			parseServerName(server);
		}
		else if (peek() == "location")
		{
			LocationConfig	tmpLocationConfig;
			size_t			i;

			tmpLocationConfig = parseLocation();
	
			i = 0;
			while (i < server.locations.size())
			{
				if (server.locations[i].path == tmpLocationConfig.path)
				{
					throw std::runtime_error("duplicate location path");
				}
				i++;
			}

			server.locations.push_back(tmpLocationConfig);
		}
		else
		{
			throw std::runtime_error("Unknown server directive: '" + peek() + "'");
		}
	}
}


ServerConfig ConfigParser::parseServer()
{
	ServerConfig	server;

	expect("server");
	expect("{");

	parseServerBody(server);

	expect("}");

	if (server.listens.empty() == true)
	{
		throw std::runtime_error("Server block must contain at least one listen directive");
	}

	return (server);
}



std::vector<ServerConfig> ConfigParser::parseConfig()
{
	tokenize();

	std::vector<ServerConfig> servers;

	while (isEnd() == false)
	{
		if (peek() == "server")
		{
			ServerConfig tmp;

			tmp = parseServer();
			servers.push_back(tmp);
		}
		else
		{
			throw std::runtime_error("Unexpected token: '" + peek() + "'\nThe .conf file must begin with 'server'");
		}
	}

	if (servers.empty() == true)
	{
		throw std::runtime_error("The configuration file must contain at least one server block");
	}

	return (servers);
}


