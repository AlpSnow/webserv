/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallis <mwallis@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 06:57:30 by mwallis           #+#    #+#             */
/*   Updated: 2026/05/02 06:57:30 by mwallis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include "Config.hpp"

#include <string>
#include <vector>
#include <cstddef> //size_t

class ConfigParser
{
	private :
		std::string					_content;
		std::vector<std::string>	_tokens;
		size_t						_pos;

		ConfigParser();

		void tokenize();

		ServerConfig parseServer();
		LocationConfig parseLocation();
		
		void parseServerBody(ServerConfig& server);
		void parseLocationBody(LocationConfig& location);

		std::string peek() const;
		std::string consume();
		void expect(const std::string& expected);
		bool isEnd() const;

		// seulement server
		void parseListen(ServerConfig& server);
		void parseServerName(ServerConfig& server);
		void parseClientMaxBodySize(ServerConfig& server);
		void parseErrorPage(ServerConfig& server);

		// server et location
		void parseRoot(std::string& root);
		void parseIndex(std::string& index);

		// seulement location
		void parseMethods(LocationConfig& location);
		void parseAutoindex(LocationConfig& location);
		void parseReturn(LocationConfig& location);
		void parseUpload(LocationConfig& location);
		void parseUploadStore(LocationConfig& location);
		void parseCgi(LocationConfig& location);
		void parseLocationClientMaxBodySize(LocationConfig& location);

	public :
		~ConfigParser();
		ConfigParser(const ConfigParser& other);
		ConfigParser& operator=(const ConfigParser& other);

		ConfigParser(const std::string& content);

		std::vector<ServerConfig> parseConfig();
};

#endif
