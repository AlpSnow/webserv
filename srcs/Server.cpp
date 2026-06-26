/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mwallis <mwallis@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/17 13:22:39 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/26 23:35:53 by mwallis          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

#define HEADER_TIMEOUT 240
#define BODY_TIMEOUT 240
#define SEND_TIMEOUT 240
#define CGI_TIMEOUT_SEC 240

#include "Server.hpp"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/wait.h>

extern volatile sig_atomic_t g_running;

Server::Server()
{
}
		
Server::~Server()
{
	closeAllFds();
}

TimeoutConfig::TimeoutConfig()
{
	headerTimeout = HEADER_TIMEOUT;
	bodyTimeout = BODY_TIMEOUT;
	sendTimeout = SEND_TIMEOUT;
}

void Server::setConfig(std::vector<ServerConfig> const &list)
{
	_configs = list;
}

bool Server::setSocketOption(int fd, int option)
{
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, option, &opt, sizeof(opt)) < 0)
		return (false);
	return (true);
}

void Server::closeAllFds()
{
	for (size_t i = 0; i < _fds.size(); i++)
	{
		if (_fds[i].fd >= 0)
			close (_fds[i].fd);
	}
	
	_fds.clear();
	_clients.clear();
	_listenFdToServers.clear();
}

std::string Server::getTime() const
{
	std::time_t now;
	char buffer[64];
	
	now = std::time(NULL);
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", 
		std::localtime(&now));
	return (std::string(buffer));
}

double Server::getResponseTime(const Client& client) const
{
	if (!client.hasStartTime)
		return (0.0);
	
	return (std::difftime(std::time(NULL), client.startTime));
}

std::string Server::methodColor(const std::string& method) const
{
	if (method == "GET")
		return (GREEN);
	else if (method == "POST")
		return (YELLOW);
	else if (method == "DELETE")
		return (RED);
	else if (method == "HEAD")
		return (CYAN);
	return (WHITE);
}

std::string Server::statusColor(int status) const
{
	if (status >= 200 && status < 300)
		return (GREEN);
	if (status >= 300 && status < 400)
		return (CYAN);
	if (status >= 400 && status < 500)
		return (YELLOW);
	if (status >= 500)
		return (RED);
	return (WHITE);
}

void Server::print_cookie(Request& request)
{
	std::map<std::string, std::string>::iterator it;

	it = request.cookies.begin();
	while (it != request.cookies.end())
	{
		std::cout << "COOKIE [" << it->first << "] = "
			<< it->second << std::endl;
		++it;
	}
}

void Server::printRecvData(const char* data, int bytes) const
{
	if (!data || bytes <= 0)
		return;

	std::cout << BOLD << MAGENTA << "recv " << RESET << bytes << " bytes" << std::endl;
	std::cout.write(data, bytes);
	std::cout << std::endl;
}

std::string Server::truncString(const std::string& str, size_t max) const
{
	if (str.size() <= max)
		return (str);
	if (max <= 3)
		return (str.substr(0, max));
	return (str.substr(0, max - 3) + "...");
}

void Server::printLog(const Client& client) const
{
	double response_time = getResponseTime(client);
	std::string method = truncString(client.request.method, 6);
	std::string path = truncString(client.request.path, 16);

	std::cout
		<< BOLD << BLUE << "● " << RESET
		<< DIM << getTime() << RESET
		<< " │ "
		<< CYAN << std::left << std::setw(15)
		<< client.ip << RESET
		<< " │ "
		<< DIM << "fd:" << std::left << std::setw(4)
		<< client.fd << RESET
		<< " │ "
		<< methodColor(client.request.method)
		<< BOLD << std::left << std::setw(6)
		<< method << RESET
		<< " "
		<< std::left << std::setw(24)
		<< path
		<< " │ "
		<< statusColor(client.statusCode)
		<< BOLD << std::setw(3)
		<< client.statusCode << RESET
		<< " │ "
		<< std::right << std::setw(5) << GREEN
		<< client.response.size() << "B" << RESET
		<< " │ "
		<< std::fixed << std::setprecision(2)
		<< response_time << "s"
		<< std::endl;
}

void Server::printRequestHeader(const Request& request) const
{
	std::string method = truncString(request.method, 8);
	std::string path = truncString(request.path + (request.query.empty() ? "" : "?" + request.query), 48);

	std::cout << BOLD << MAGENTA << "→ " << RESET
			  << methodColor(request.method) << std::left << std::setw(8) << method << RESET
			  << " " << std::left << std::setw(48) << path
			  << " " << DIM << request.http_version << RESET
			  << std::endl;

	int printed = 0;
	for (std::map<std::string, std::string>::const_iterator it = request.header.begin();
		 it != request.header.end() && printed < 5; ++it, ++printed)
	{
		std::string key = it->first;
		std::string val = it->second;
		std::cout << "  " << CYAN << key << RESET << ": " << truncString(val, 120) << std::endl;
	}
	if (request.header.size() > (size_t)printed)
		std::cout << "  " << DIM << "... (" << (request.header.size() - printed) << " more headers)" << RESET << std::endl;
}

void Server::printResponseHeader(const HttpResponse& response) const
{
	int status = response.get_http_status();
	size_t remaining = response.get_remaining_bytes();

	std::cout << BOLD << MAGENTA << "← " << RESET
			  << statusColor(status) << std::setw(3) << status << RESET
			  << " │ " << std::right << remaining << "B remaining" << RESET
			  << std::endl;

	const char* data = response.get_response_data();
	size_t len = remaining;
	if (data && len > 0)
	{
		size_t peek_len = (len > 1024 ? 1024 : len);
		std::string s(data, data + peek_len);
		size_t pos = s.find("\r\n\r\n");
		std::string headers = (pos != std::string::npos) ? s.substr(0, pos) : s;

		std::istringstream iss(headers);
		std::string line;
		while (std::getline(iss, line))
		{
			if (!line.empty() && line[line.size() - 1] == '\r')
				line.resize(line.size() - 1);
			std::cout << "  " << DIM << line << RESET << std::endl;
		}
		if (pos == std::string::npos && len > peek_len)
			std::cout << "  " << DIM << "... (truncated)" << RESET << std::endl;
	}
}

std::string Server::makeListenKey(const std::string& host, int port)
{
	std::ostringstream oss;

	oss << host << ":" << port;
	return (oss.str());
}

bool Server::initServer()
{
	for (size_t i = 0; i < _configs.size(); i++)
	{
		for (size_t j = 0; j < _configs[i].listens.size(); j++)
		{
			std::string host = _configs[i].listens[j].host;
			int port = _configs[i].listens[j].port;
			std::string key = makeListenKey(host, port);

			if (_listenKeyToFd.find(key) != _listenKeyToFd.end())
			{
				int existingFd = _listenKeyToFd[key];

				_listenFdToServers[existingFd].push_back(&_configs[i]);
				continue;
			}
			
			//AF_INET IPv4
			//SOCK_STREAM connexion TCP
			int serverFd = socket(AF_INET, SOCK_STREAM, 0);
			if (serverFd < 0)
			{
				std::cerr << "Erreur socket" << std::endl;
				return (false);
			}
			
			// On permet au serveur de se relancer si le port est en cours de fermeture
			if (!setSocketOption(serverFd, SO_REUSEADDR))
			{
				std::cerr << "Erreur setsockopt" << std::endl;
				close(serverFd);
				serverFd = -1;
				return (false);
			}

			sockaddr_in address;
			memset(&address, 0, sizeof(address));
			
			address.sin_family = AF_INET;
			address.sin_port = htons(_configs[i].listens[j].port);
			//_serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");//htonl(INADDR_ANY);
			address.sin_addr.s_addr = inet_addr(_configs[i].listens[j].host.c_str());
			
			// On attache le socket a une adresse 
			std::cout << "Trying bind: "
				<< YELLOW
          		<< _configs[i].listens[j].host 
				<< RESET
				<< ":"
				<< CYAN
          		<< _configs[i].listens[j].port
				<< RESET
          		<< std::endl;
			if (bind(serverFd, (struct sockaddr *)&address, 
				sizeof(address)) < 0)
			{
				std::cerr << "Erreur bind on "
          			<< _configs[i].listens[j].host << ":"
          			<< _configs[i].listens[j].port
          			<< " -> " << strerror(errno) << std::endl;
				close(serverFd);
				serverFd = -1;
				return (false);
			}
			
			// Le socket passe en mode serveur (écoute) et accepete des connexions
			if (listen(serverFd, SOMAXCONN) < 0)
			{
				std::cerr << "Erreur listen" << std::endl;
				close(serverFd);
				serverFd = -1;
				return (false);
			}
	
			// On récupére les flags et on passe et on ajoute le non bloquant
			int flags = fcntl(serverFd, F_GETFL, 0);
			if (flags < 0 || fcntl(serverFd, F_SETFL, flags | O_NONBLOCK) < 0)
			{
				std::cerr << "Erreur fcntl" << std::endl;
				close(serverFd);
				serverFd = -1;
				return (false);
			}
			_listenKeyToFd[key] = serverFd;
			_listenFdToServers[serverFd].push_back(&_configs[i]);
			initPoll(serverFd);
		}
	}
	return (true);
}

void Server::removeClient(int i)
{
	close(_fds[i].fd);
	_clients.erase(_fds[i].fd);
	_fds.erase(_fds.begin() + i);
}

void Server::initPoll(int fd)
{
	pollfd server_poll_fd;
	server_poll_fd.fd = fd;
	server_poll_fd.events = POLLIN;
	server_poll_fd.revents = 0;
	_fds.push_back(server_poll_fd);
}

bool Server::checkPoll()
{
	if (_fds.empty())
		return (false);

	int ret = poll(&_fds[0], _fds.size(), 1000); // 1000 (every second server check activity)
	if (ret < 0)
	{
		if (errno == EINTR) // Need utility check (remettre errno a 0 apres ?)
		{
			std::cout << "\n" GREEN "Arret du serveur..." RESET << std::endl;
			return (true);
		} // if block no util comment
		std::cerr << "Erreur poll" << std::endl;
		return (false);
	}
	return (true);
}

std::string Server::ipToString(unsigned int ip) const
{
	std::ostringstream oss;
	
	ip = ntohl(ip); // Network to host long 

	oss << ((ip >> 24) & 0xFF) << "."
		<< ((ip >> 16) & 0xFF) << "."
		<< ((ip >> 8) & 0xFF) << "."
		<< (ip & 0xFF);
	
	return (oss.str());
}

int Server::getTimeoutClient(const Client& client, short events) const
{
	RequestState state = client.parser.get_status();

	if (events & POLLOUT)
		return (_timeoutConfig.sendTimeout);
		
	if (state == READING_HEADER || state == READING_REQUEST)
		return (_timeoutConfig.headerTimeout);
	
	if (state == READING_BODY || state == READING_CHUNKED)
		return (_timeoutConfig.bodyTimeout);
	
	return (_timeoutConfig.headerTimeout);
}

std::string Server::buildTimeoutResponse() const
{
	return (
		"HTTP/1.1 408 Request Timeout\r\n"
		"Content-Length: 15\r\n"
		"Content-Type: text/plain\r\n"
		"Connection: close\r\n"
		"\r\n"
		"Request Timeout"
	);
}

bool Server::handleTimeout(int i)
{
	Client &client = _clients[_fds[i].fd];

	if (_fds[i].events & POLLOUT)
	{
		removeClient(i);
		return (true);
	}
	
	client.statusCode = 408;
	client.response = buildTimeoutResponse();
	client.bytesSent = 0;
	
	if (!client.hasStartTime)
	{
		client.startTime = std::time(NULL);
		client.hasStartTime = true;
	}
	
	printLog(client);

	_fds[i].events = POLLOUT;
	client.lastActivity = std::time(NULL); // evite de timeout avant reponse
	
	return (false);
}

void	Server::remove_cgi_fd(Client& client)
{
    for (int j = 0; j < (int)_fds.size(); j++)
    {
        if ((client.cgi.pipe_in != -1 && _fds[j].fd == client.cgi.pipe_in) || 
            (client.cgi.pipe_out != -1 && _fds[j].fd == client.cgi.pipe_out))
        {
            close(_fds[j].fd);
            client_pipe.erase(_fds[j].fd);
            _fds.erase(_fds.begin() + j);
            j--; // Adjust index after erase
        }
        else if (_fds[j].fd == client.fd)
        {
            _fds[j].events = POLLOUT; // Wake up the client socket
        }
    }
    client.cgi.pipe_in = -1;
    client.cgi.pipe_out = -1;
}

void Server::checkTimeouts()
{
	time_t now = std::time(NULL);
	
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        Client& client = it->second;
        if (client.cgi.stats == CGI_READING || client.cgi.stats == CGI_WRITING)
        {
            if (now - client.cgi.start_time >= CGI_TIMEOUT_SEC)
            {
                std::cerr << "CGI Timeout detected for client " << client.fd << "!" << std::endl;
                
                // 1. Kill the stuck process
                kill(client.cgi.cgi_pid, SIGKILL);
                waitpid(client.cgi.cgi_pid, NULL, 0);

                client.cgi.stats = CGI_ERROR;

                const ServerConfig* active_config = getMatchedServer(client.request.get_host(), client.listenFd);
                client.response_builder.generate_error(504, active_config, client.request);
                client.statusCode = 504;

               remove_cgi_fd(client);
            }
        }
    }
	for (int i = 0; i < (int)_fds.size(); i++)
	{
		int fd = _fds[i].fd;
		
		if (_listenFdToServers.find(fd) != _listenFdToServers.end())
			continue;

		if (_clients.find(fd) == _clients.end())
			continue;
		
		int timeout = getTimeoutClient(_clients[fd], _fds[i].events);
		
		if (now - _clients[fd].lastActivity >= timeout)
		{
			if (handleTimeout(i))
				i--;
		}
	}
}

void Server::acceptClient(int listenFd)
{
	sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	
	int client_fd = accept(listenFd, (struct sockaddr *)&client_addr,
		&client_len);
	if (client_fd < 0)
		return;
	// std::cout << "Client accepted on listenFd: " << listenFd << std::endl;

	int flags = fcntl(client_fd, F_GETFL, 0);
	if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(client_fd);
		return;
	}

	pollfd client_poll_fd;
	client_poll_fd.fd = client_fd;
	client_poll_fd.events = POLLIN;
	client_poll_fd.revents = 0;
	_fds.push_back(client_poll_fd);

	_clients[client_fd] = Client(client_fd);
	_clients[client_fd].listenFd = listenFd;
	_clients[client_fd].ip = ipToString(client_addr.sin_addr.s_addr);
	// std::cout << "Client ajouté dans poll" << std::endl;
}

const ServerConfig* Server::getMatchedServer(const std::string& host_header, int listenFd) const
{
	std::string host = host_header;
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos)
        host = host.substr(0, colon_pos);

    std::map<int, std::vector<const ServerConfig*> >::const_iterator it = _listenFdToServers.find(listenFd);
    
    if (it == _listenFdToServers.end() || it->second.empty())
        return (NULL); 

    const std::vector<const ServerConfig*>& port_configs = it->second;

    for (size_t i = 0; i < port_configs.size(); ++i)
    {
        for (size_t j = 0; j < port_configs[i]->serverNames.size(); ++j)
        {
            if (port_configs[i]->serverNames[j] == host)
                return (port_configs[i]); // Found a match on this port!
        }
    }

    return (port_configs[0]);
}

bool Server::handleClient(int i)
{
	Client &client = _clients[_fds[i].fd];
	RequestState state;
	char buffer[1024];

	int bytes = recv(_fds[i].fd, buffer, sizeof(buffer), 0);
	
	if (bytes < 0)
	{
		// std::cout << "recv < 0 bytes" << std::endl;
		removeClient(i);
		return (true);
	}
	else if (bytes > 0)
	{
		// printRecvData(buffer, bytes);
		client.lastActivity = std::time(NULL);
		if (!client.hasStartTime)
		{
			client.startTime = std::time(NULL);
			client.hasStartTime = true;
		}
		const ServerConfig* request_config = getMatchedServer("", client.listenFd);
		client.parser.parse_chunk(buffer, bytes, client.request, request_config);
	}

	state = client.parser.get_status();
    
	if (bytes == 0)
	{
		// std::cout << "recv = 0 bytes" << std::endl;
		if (state != COMPLETE)
		{
			// std::cout << "status != COMPLETE" << std::endl;
			removeClient(i);
			return (true);
		}
	}

    const ServerConfig* active_config = getMatchedServer(client.request.get_host(), client.listenFd);

	if (state == ERROR)
	{	
		client.response_builder.generate_error(client.parser.get_error_code(), active_config, client.request);
		client.isKeepAlive = client.request.keep_alive;
		client.statusCode = client.response_builder.get_http_status();
		printLog(client);
		_fds[i].events = POLLOUT;
		return (false);
	}

	if (state == COMPLETE)
	{
		// printRequestHeader(client.request);
		// print_cookie(client.request);
		client.response_builder.generate(client.request, active_config, client, _sessionManager);
		client.isKeepAlive = client.request.keep_alive;
		client.statusCode = client.response_builder.get_http_status();
		printLog(client);
		if (client.cgi.stats == CGI_READING || client.cgi.stats == CGI_WRITING)
		{
			connect_cgi_to_poll(client);
			_fds[i].events = 0;
		}
		else
			_fds[i].events = POLLOUT;
		return (false);
	}
	return (false);
}

void	Server::connect_cgi_to_poll(Client& client)
{
	if (client.cgi.stats == CGI_WRITING)
	{
		pollfd	p_fd_in;
		p_fd_in.fd = client.cgi.pipe_in;
		p_fd_in.events = POLLOUT;
		p_fd_in.revents = 0;

		_fds.push_back(p_fd_in);
		client_pipe[client.cgi.pipe_in] = client.fd;
	}

	pollfd	p_fd_out;
	p_fd_out.fd = client.cgi.pipe_out;
	p_fd_out.events = POLLIN;
	p_fd_out.revents = 0;

	_fds.push_back(p_fd_out);
	client_pipe[client.cgi.pipe_out] = client.fd;
}

void Server::resetClientForNextRequet(Client& client)
{
	client.parser = ClientRequest();
	client.request.clear();
	client.hasStartTime = false;
	client.bytesSent = 0;
	client.statusCode = 0;
	client.isKeepAlive = true;
	client.lastActivity = std::time(NULL);
}

bool	Server::sendResponse(int i)
{
	Client &client = _clients[_fds[i].fd];
	
	size_t		bytes_to_send = client.response_builder.get_remaining_bytes();
	// if (client.response_builder.get_state() == RESPONSE_READY_TO_SEND)
	// 	printResponseHeader(client.response_builder);
	
	int ret = send(_fds[i].fd, client.response_builder.get_response_data(), bytes_to_send, 0);
	if (ret <= 0)
	{
		removeClient(i);
		return (true);
	}
	client.response_builder.add_bytes_sent(ret);

	client.lastActivity = std::time(NULL);

	if (client.response_builder.get_state() == RESPONSE_COMPLETE)
	{
		if (!client.isKeepAlive)
		{
			std::cout << "Not keep alive" << std::endl;
			removeClient(i);
			return (true);
		}
		resetClientForNextRequet(client);
		_fds[i].events = POLLIN;
	}
	return (false);
}

bool	Server::handle_sending_data_to_cgi(int i)
{
	int pipe_fd_in = _fds[i].fd;
	Client &client = _clients[client_pipe[pipe_fd_in]];

	if (client.cgi.stats == CGI_WRITING)
	{
		int bytes_writted = write(pipe_fd_in, client.request.body.data() + client.cgi.bytes_sended,
								client.request.body.size() - client.cgi.bytes_sended);
		
		if (bytes_writted < 0)
		{
			client.cgi.stats = CGI_ERROR;
			remove_cgi_fd(client);
			return (true);
		}

		client.cgi.bytes_sended += bytes_writted;

		if (client.cgi.bytes_sended >= client.request.body.size())
		{
			close(pipe_fd_in);
			client_pipe.erase(pipe_fd_in);
			if (client.cgi.stats != CGI_FINISHED)
				client.cgi.stats = CGI_READING;
			_fds.erase(_fds.begin() + i);
			return (true);
		}
	}
	return (false);
}

bool	Server::handle_reading_data_from_cgi(int i)
{
	int pipe_fd_out = _fds[i].fd;
	Client &client = _clients[client_pipe[pipe_fd_out]];
	char	buffer[1024];

	if (client.cgi.stats == CGI_READING || client.cgi.stats == CGI_WRITING)
	{
		int	bytes_readed = read(pipe_fd_out, buffer, sizeof(buffer));

		if (bytes_readed < 0)
		{
			client.cgi.stats = CGI_ERROR;
			remove_cgi_fd(client);
			return (true);
		}
		else if (bytes_readed == 0)
		{
			waitpid(client.cgi.cgi_pid, NULL, 0);
			client.cgi.stats = CGI_FINISHED;
			client.response_builder.parse_cgi_output(client.cgi.response_buffer, client.request);
			remove_cgi_fd(client);
			return (true);
		}
		else
		{
			client.cgi.response_buffer.insert(client.cgi.response_buffer.end(),
                                              buffer, buffer + bytes_readed);
		}
	}
	return (false);
}

bool Server::handleEvents()
{
	for (int i = 0; i < (int)_fds.size(); i++)
	{
		if (_listenFdToServers.find(_fds[i].fd) != _listenFdToServers.end())
		{
			if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
			{
				std::cerr << "Erreur sur le socket serveur" << std::endl;
				return (false);
			}
			if (_fds[i].revents & POLLIN)
				acceptClient(_fds[i].fd);
		}
		else
		{
			bool removed = false;
			if (_fds[i].revents & (POLLERR | POLLNVAL))
			{
				//std::cout << "Caught POLLHUP or other" << std::endl;
				removeClient(i);
				removed = true;
			}
			if (client_pipe.find(_fds[i].fd) != client_pipe.end())
			{
                if (_fds[i].revents & POLLOUT)
					removed = handle_sending_data_to_cgi(i);
				else if (_fds[i].revents & (POLLIN | POLLHUP))
					removed = handle_reading_data_from_cgi(i);
			}
			else {
				if (!removed && (_fds[i].revents & POLLIN))
					removed = handleClient(i);
				if (!removed && (_fds[i].revents & POLLOUT))
					removed = sendResponse(i);
				if (!removed && (_fds[i].revents & POLLHUP)
					&& !(_fds[i].events & POLLOUT))
				{
					removeClient(i);
					removed = true;
				}
			}
			if (removed)
				i--;
		}
	}
	return (true);
}

void Server::run()
{
	std::cout << GREEN "Lancement du serveur..." RESET << std::endl;
	while (g_running)
	{
		if (!checkPoll())
			break;
		if (!handleEvents())
			break;
		checkTimeouts();
	}
	closeAllFds();
}
