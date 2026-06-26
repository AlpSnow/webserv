/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 17:15:28 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/15 14:23:15 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config_loader/Config.hpp"
#include "srcs/Server.hpp"
#include <csignal>
#include <iostream>
#include <exception>
// #include <string>   au cas ou

volatile sig_atomic_t g_running = 1;
//volatile dit au compilateur
//“cette variable peut changer à tout moment en dehors du flux normal du code”

void handle_signal(int signal)
{
	(void)signal;
	g_running = 0;	
}


static bool getPath(int argc, char *argv[], std::string& path)
{
    if (argc == 1)
    {
        path = "configs/default.conf";
    }
    else if (argc == 2)
    {
        path = std::string(argv[1]);
    }
    else 
    {
        std::cerr << "Error:\n"
                  << "Usage:\n"
                  << "  ./webserv [configuration_file.conf]\n"
                  << "  ./webserv (uses default: configs/default.conf)"
                  << std::endl;

        return (false);
    }
    return (true);
}


static bool checkConfExtension(const std::string& path)
{
    if (path.size() < 5 || path.substr(path.size() - 5) != ".conf")
    {
        std::cerr << "Error: file must be .conf" << std::endl;
        return (false);
    }
    return (true);
}

// static void printLocationCgi(const std::vector<ServerConfig>& servers)
// {
//     for (size_t serverIndex = 0; serverIndex < servers.size(); ++serverIndex)
//     {
//         const ServerConfig& server = servers[serverIndex];

//         for (size_t locationIndex = 0; locationIndex < server.locations.size(); ++locationIndex)
//         {
//             const LocationConfig& location = server.locations[locationIndex];

//             if (location.cgi.empty() == true)
//             {
//                 continue;
//             }

//             std::cout << "server[" << serverIndex << "] location[" << locationIndex << "] path="
//                       << location.path << std::endl;
//             for (std::map<std::string, std::string>::const_iterator it = location.cgi.begin();
//                  it != location.cgi.end(); ++it)
//             {
//                 std::cout << "  " << it->first << " -> " << it->second << std::endl;
//             }
//         }
//     }
// }

int main(int argc, char *argv[])
{
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
    // signal(SIGPIPE, SIG_IGN);
	Server server;
	
    std::string path;

    if (getPath(argc, argv, path) == false)
    {
        return (1);
    }
    if (checkConfExtension(path) == false)
    {
        return (1);
    }

    try
    {
        Config config(path);
		// printLocationCgi(config.getServers());
		server.setConfig(config.getServers());
    }
    catch (const std::exception& e)
    {
        std::cerr << "Config error: " << e.what() << std::endl;
        return (1);
    }
	
	if (!server.initServer())
		return (1);
	server.run();


	return (0);
}



// ============================================================================
// GROUPE !!!
// ============================================================================

// 1. client_max_body_size dans location :
// ============================================================================
//
// Ne jamais utiliser directement :
//
//     location.clientMaxBodySize
//
// sans vérifier avant :
//
//     location.hasClientMaxBodySize
//
// Une location peut redéfinir la valeur du server.
//
// Exemple :
//
// server {
//     client_max_body_size 1000000;
//
//     location /upload {
//         client_max_body_size 5000000;
//     }
// }
//
// Logique à utiliser dans le serveur :
//
// size_t effectiveMaxBodySize;
//
// if (location.hasClientMaxBodySize == true)
// {
//     effectiveMaxBodySize = location.clientMaxBodySize;
// }
// else
// {
//     effectiveMaxBodySize = server.clientMaxBodySize;
// }
// ============================================================================



// 2. actuellement un seul index est accepté. Mais si vous voulez on peut en faire plusieurs :
// ============================================================================
//
// Un seul fichier index est accepté.
//
// Valide :
//
//     index index.html;
//
// Invalide :
//
//     index index.htm index.html;
//
// ============================================================================



// 3. Notes importantes pour vous plus tard :
// ============================================================================
//
// 1. DOUBLONS listen ENTRE PLUSIEURS SERVER
// ----------------------------------------------------------------------------
//
// Actuellement, les doublons listen sont interdits uniquement
// dans un même bloc server.
//
// Invalide :
//
//     server {
//         listen 8080;
//         listen 8080;
//     }
//
// Mais ceci est actuellement accepté :
//
//     server {
//         listen 8080;
//         root site1;
//     }
//
//     server {
//         listen 8080;
//         root site2;
//     }
//
// Ce n'est pas forcément faux.
//
// Cela dépendra de l'architecture réseau finale.
//
// - Si un bind/socket est créé pour chaque ServerConfig,
//   alors il faudra probablement interdire globalement
//   les doublons host:port.
//
// - Si un seul socket est partagé pour un host:port,
//   puis que le bon server est choisi avec server_name,
//   alors ce comportement peut être autorisé.
//
// Le sujet autorise les virtual hosts si souhaité.
// Ce point dépend donc de l'architecture réseau finale,
// pas du parser.
//
// ----------------------------------------------------------------------------
//
// 2. listen 0.0.0.0:8080 + listen 127.0.0.1:8080
// ----------------------------------------------------------------------------
//
// Le parser accepte actuellement :
//
//     server {
//         listen 0.0.0.0:8080;
//         listen 127.0.0.1:8080;
//     }
//
// Syntaxiquement, les deux valeurs sont différentes,
// donc le contrôle de doublon ne les bloque pas.
//
// Cependant :
//
//     0.0.0.0:8080
//
// signifie : "toutes les interfaces réseau".
//
// Selon la gestion finale des sockets,
// cela peut provoquer un conflit au moment du bind.
//
// Ce n'est probablement pas au parser de gérer ce cas,
// mais l'équipe devra faire attention à ce point
// dans la partie réseau du projet.
// ============================================================================
