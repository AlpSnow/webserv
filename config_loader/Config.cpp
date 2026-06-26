/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallis <mwallis@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 06:57:23 by mwallis           #+#    #+#             */
/*   Updated: 2026/05/02 06:57:23 by mwallis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "ConfigParser.hpp"

#include <fstream>   // ifstream                 (ouvrir et lire un fichier)
#include <sstream>   // stringstream, rdbuf()    (mettre le contenu du fichier en mémoire + transformer fichier en string)
#include <stdexcept> // throw, runtime_error     (gérer les erreurs)
#include <iostream>

Config::Config()
{

}

Config::~Config()
{

}

Config::Config(const Config& other)
: _servers(other._servers)
{

}

Config& Config::operator=(const Config& other)
{
    if (this != &other)
    {
        this->_servers = other._servers;
    }
    return (*this);
}

Config::Config(const std::string& path)
{
    std::ifstream   fileToRead;

    fileToRead.open(path.c_str());
    if (fileToRead.is_open() == false)
    {
        throw std::runtime_error("Cannot open config file");
    }

    std::stringstream   tmp;
    std::string         contentFile;

    tmp << fileToRead.rdbuf();
    contentFile = tmp.str();

    ConfigParser serverConfigParser(contentFile);

    this->_servers = serverConfigParser.parseConfig();
    fileToRead.close();
}

const std::vector<ServerConfig>& Config::getServers() const
{
    return (this->_servers);
}










    // ------------------------------------------------------
    // std::stringstream tmp;
    // ------------------------------------------------------

    // std::stringstream : un flux (stream) en mémoire qui fonctionne comme un fichier

    // std::ifstream     -> lit depuis un fichier
    // std::stringstream -> lit depuis une string (en mémoire)
    // std::string       -> variable string

    // ifstream     -> lit un fichier
    // stringstream -> stocke et manipule du texte en mémoire
    // string       -> contient le texte final

    
    // ------------------------------------------------------
    // tmp << fileToRead.rdbuf();
    // ------------------------------------------------------

    // rdbuf() donne accès au contenu brut d’un flux (ex: fichier)
    // file.rdbuf() représente tout le contenu du fichier
    // buffer << file.rdbuf() copie tout le fichier dans le stringstream
    // permet de lire un fichier en une seule opération

    // rdbuf() retourne un objet interne (stream buffer)
    // qui contient tout le contenu du flux (ici, le fichier)