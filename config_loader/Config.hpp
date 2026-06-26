/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallis <mwallis@student.42mulhouse.fr>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/02 06:57:26 by mwallis           #+#    #+#             */
/*   Updated: 2026/05/02 06:57:26 by mwallis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstddef> // size_t

struct ListenConfig
{
    std::string                         host;
    int                                 port;

    ListenConfig() : host("0.0.0.0"), port(0) {}
};

struct LocationConfig
{
    std::string                         path;
    std::string                         root;
    std::string                         index;
    std::set<std::string>               methods;
    bool                                hasMethods;
    bool                                autoindex;
    bool                                hasAutoindex;
    size_t                              clientMaxBodySize;
    bool                                hasClientMaxBodySize;

    bool                                hasRedirect;
    int                                 redirectCode;
    std::string                         redirectTarget;

    bool                                uploadEnabled;
    bool                                hasUpload;
    std::string                         uploadStore;

    std::map<std::string, std::string>  cgi;

    LocationConfig()
        : hasMethods(false),
          autoindex(false),
          hasAutoindex(false),
          clientMaxBodySize(0),
          hasClientMaxBodySize(false),
          hasRedirect(false),
          redirectCode(0),
          uploadEnabled(false),
          hasUpload(false)
    {
    }
};

struct ServerConfig
{
    std::vector<ListenConfig>           listens;
    std::vector<std::string>            serverNames;
    std::string                         root;
    std::string                         index;
    size_t                              clientMaxBodySize;
    bool                                hasClientMaxBodySize;
    std::map<int, std::string>          errorPages;
    std::vector<LocationConfig>         locations;

    ServerConfig()
        : clientMaxBodySize(1000000),
          hasClientMaxBodySize(false)
    {
    }
};

class Config
{
    private :
        std::vector<ServerConfig>   _servers;

        Config();

    public :
        ~Config();
        Config(const Config& other);
        Config& operator=(const Config& other);

        Config(const std::string& path);

        const std::vector<ServerConfig>& getServers() const;
};

#endif










// En C++, struct et class sont presque identiques 
// (seule différence : public pour les struct vs private pour les classes par défaut) 
// Une struct peut donc avoir un constructeur 
// LocationConfig() : autoindex(false) {} initialise autoindex à false 
// C'est une liste d'initialisation (bonne pratique en C++) 
// -> totalement normal et recommandé à utiliser


// ------------------------------------------------------
// struct ListenConfig
// {
//     std::string host;
//     int         port;
// };
// ------------------------------------------------------

// Un serveur réseau écoute toujours sur une combinaison : IP (host) + PORT
// -> Les deux sont obligatoires pour créer et binder un socket

// host = adresse IP sur laquelle le serveur écoute
// -> Définit sur quelle interface réseau le serveur accepte des connexions
// -> Adresse IP sur laquelle le serveur écoute

// port = numéro du service (ex: 8080, 80, 443)
// -> Permet d’identifier le service côté machine (HTTP, HTTPS, etc.)

// Exemple 1 :
// "listen 8080;"
// -> L’utilisateur ne précise pas l’IP
// -> On utilise par défaut : host = "0.0.0.0"
// -> Donc le serveur écoute sur toutes les interfaces réseau
// -> Résultat : host = "0.0.0.0", port = 8080

// Exemple 2 :
// "listen 127.0.0.1:8080;"
// -> L’utilisateur précise l’IP
// -> Le serveur écoute uniquement en local (localhost)
// -> Résultat : host = "127.0.0.1", port = 8080

// Ces deux valeurs sont ensuite utilisées pour configurer le socket :
// -> host → sin_addr.s_addr (adresse IP)
// -> port → sin_port (port réseau)


// Pourquoi host ET port ?

// Parce qu’un serveur écoute sur :

// IP + PORT

// Le port = numéro de service :
// 8080, 80, 443…

// ça dit quel programme sur la machine

// Le host (IP) = où écouter
// 127.0.0.1   → seulement sur ta machine (localhost)
// 0.0.0.0     → partout (toutes les interfaces réseau)
// 192.168.x.x → réseau local

// ça dit “depuis où on peut accéder au serveur”






// ------------------------------------------------------
// struct LocationConfig
// {
//     std::string                 path;
//     std::string                 root;
//     std::string                 index;
//     std::set<std::string>       methods;
//     bool                        autoindex;
// };
// ------------------------------------------------------


// ------------------------------------------------------
// path
// ------------------------------------------------------
// URL prefix

// Exemples :
// location /        → path = "/"
// location /images  → path = "/images"

// Sert à matcher une requête :
// /images/cat.png → match "/images"


// ------------------------------------------------------
// root
// ------------------------------------------------------
// Dossier réel sur le disque

// Exemple :
// root ./www;

// Requête :
// /images/cat.png

// Fichier :
// ./www/images/cat.png


// ------------------------------------------------------
// index
// ------------------------------------------------------
// Fichier par défaut si la ressource est un dossier

// Exemple :
// index index.html;

// Requête :
// /

// Sert :
// ./www/index.html


// ------------------------------------------------------
// methods
// ------------------------------------------------------
// Méthodes HTTP autorisées

// Exemple :
// methods GET POST DELETE;

// Résultat :
// {"GET", "POST", "DELETE"}

// Si une requête utilise une méthode interdite → 405


// ------------------------------------------------------
// autoindex
// ------------------------------------------------------
// Listing automatique du contenu d’un dossier

// Exemple :
// autoindex on;

// Si aucun fichier index n’est présent :
// → afficher la liste des fichiers

// Sinon :
// → erreur ou aucune réponse spécifique






// ------------------------------------------------------
// struct ServerConfig
// {
//     std::vector<ListenConfig>  listens;
//     std::string                root;
//     std::string                index;
//     size_t                     clientMaxBodySize;
//     std::map<int, std::string> errorPages;
//     std::vector<LocationConfig> locations;
// };
// ------------------------------------------------------


// ------------------------------------------------------
// listens
// ------------------------------------------------------
// Liste des ports / hosts sur lesquels le serveur écoute

// Exemple :
// listen 8080;
// listen 127.0.0.1:8081;

// Résultat :
// [ ("0.0.0.0", 8080), ("127.0.0.1", 8081) ]


// ------------------------------------------------------
// root (server)
// ------------------------------------------------------
// Dossier racine global du serveur

// Hérité par les locations si elles ne définissent pas leur propre root

// Exemple :
// server {
//     root ./www;
// }


// ------------------------------------------------------
// index (server)
// ------------------------------------------------------
// Fichier par défaut global

// Utilisé comme fallback pour les locations

// Exemple :
// index index.html;


// ------------------------------------------------------
// clientMaxBodySize
// ------------------------------------------------------
// Taille maximale autorisée pour le body d’une requête HTTP

// Exemple :
// client_max_body_size 1000000;

// Si la requête dépasse cette taille → erreur 413 (Payload Too Large)


// ------------------------------------------------------
// errorPages
// ------------------------------------------------------
// Pages d’erreur personnalisées

// Exemple :
// error_page 404 /errors/404.html;

// Résultat :
// 404 → "/errors/404.html"


// ------------------------------------------------------
// locations
// ------------------------------------------------------
// Ensemble des routes définies pour le serveur

// Chaque location contient sa propre configuration


// ------------------------------------------------------
// Héritage server → location
// ------------------------------------------------------
// Les locations héritent des valeurs du server si elles ne les redéfinissent pas

// Exemple :
// server {
//     root ./www;

//     location /images {
//      (pas de root ici)
//     }
// }

// Résultat :
// /images utilise root = ./www