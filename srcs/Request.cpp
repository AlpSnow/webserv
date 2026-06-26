/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 14:43:26 by cnamoune          #+#    #+#             */
/*   Updated: 2026/06/17 17:36:36 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include "RequestCookies.hpp"
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <limits.h>

std::string	Request::get_host() const
{
    if (header.find("Host") != header.end())
		return (header.at("Host"));
	return ("");
}

ClientRequest::ClientRequest()
{
	status = READING_REQUEST;
	content_length = 0;
	body_bytes_readed = 0;
	file_fd = -1;
	http_error_code = 0;
	current_data_size = 0;
	current_data_size_readed = 0;
	reading_data_chunked = false;
	crlf_received = false;
	config = NULL;
}

ClientRequest::~ClientRequest()
{
	
}

Request::Request()
{
    this->method = "";
    this->path = "";
	keep_alive = true;
    this->location_match = NULL; 
}

void    Request::clear()
{
    this->method.clear();
    this->path.clear();
    header.clear();
    body.clear();
    this->location_match = NULL;
	cookies.clear();
}

Request::~Request()
{
	this->method.clear();
    this->path.clear();
	header.clear();
	body.clear();
    this->location_match = NULL;
	cookies.clear();
}

void Request::print_body() const
{
	if (!body.empty())
		std::cout.write(body.data(), body.size());
	std::cout << std::endl;
}

void ClientRequest::parse_request_line(Request& request)
{
    size_t		request_length;
	std::string	line;

	request_length = data.find("\r\n");
	if (request_length == std::string::npos)
		return ;
	line = data.substr(0, request_length);

	data.erase(0, request_length + 2);
	std::istringstream	iss(line);
	std::string			extra;
	iss >> request.method >> request.path >> request.http_version;
	if (request.method.empty() || request.path.empty()
	|| request.http_version.empty() || (iss >> extra))
	{
		status = ERROR;
		set_error_code(400);
		return ;
	}
	if (request.method != "GET" && request.method != "HEAD"
		&& request.method != "POST" && request.method != "DELETE")
	{
		status = ERROR;
		set_error_code(501);
		return ;
	}
	if (request.http_version != "HTTP/1.1")
	{
		status = ERROR;
		set_error_code(505);
		return ;
	}
	size_t	query_pos = request.path.find("?");
	if (query_pos != std::string::npos)
	{
		request.query = request.path.substr(query_pos + 1);
    	request.path = request.path.substr(0, query_pos);
	}
	status = READING_HEADER;
}

bool ClientRequest::is_keep_alive(Request& request)
{
    std::map<std::string, std::string>::iterator it;

    it = request.header.find("Connection");
    if (it != request.header.end() && !it->second.empty() && it->second == "close")
        request.keep_alive = false;

    return (request.keep_alive);
}

void			ClientRequest::parse_body(Request& request)
{
	size_t bytes_to_read;

	if (body_bytes_readed > content_length)
	{
		set_error_code(400);
		status = ERROR;
		return ;
	}
    if (body_bytes_readed == content_length)
	{	
        status = COMPLETE;
		return ;
    }
	
	bytes_to_read = content_length - body_bytes_readed;
	
	if (data.size() < bytes_to_read)
		bytes_to_read = data.size();

	request.body.insert(request.body.end(), data.begin(), data.begin() + bytes_to_read);
	data.erase(0, bytes_to_read);
	body_bytes_readed += bytes_to_read;
	if (body_bytes_readed == content_length)
	{
		is_keep_alive(request);
		status = COMPLETE;
	}
}

long	ClientRequest::get_chunk_size(const std::string& hex_size)
{
	if (hex_size == "WAIT")
		return (-2);
	
	if (hex_size.empty())
	{
		status = ERROR;
		set_error_code(400);
		return (-1);
	}

	if (!std::isxdigit(static_cast<unsigned char>(hex_size[0])))
	{
		status = ERROR;
		set_error_code(400);
		return (-1);
	}
	long	size;
	char	*end;

	size = strtol(hex_size.c_str(), &end, 16);
	if (size < 0 || (*end != '\0' && *end != ';'))
	{
		status = ERROR;
		set_error_code(400);
		return (-1);
	}
	return (size);
}

std::string	ClientRequest::extract_chunk_size()
{
	size_t	end_of_line = data.find("\r\n");
	
	if (end_of_line == std::string::npos)
		return ("WAIT");
	
	std::string	hex_size = data.substr(0, end_of_line);
	data.erase(0, end_of_line + 2);
	return (hex_size);
}

int	ClientRequest::is_crlf_correct(int code)
{
	if (data.size() < 2)
		return (0);

	if (data[0] != '\r' || data[1] != '\n')
	{
		set_error_code(400);
		status = ERROR;
		return (-1);
	}

	data.erase(0, 2);

	current_data_size = 0;
	current_data_size_readed = 0;
	reading_data_chunked = false;

	if (code == 0)
	{
		crlf_received = false;
		status = COMPLETE;
	}

	return (1);
}

bool	ClientRequest::check_the_size(const Request& request)
{
	if (request.body.size() > 100000001)
		return (true);
	if (config && config->hasClientMaxBodySize)
	{
		if (request.body.size() > config->clientMaxBodySize)
			return (true);
	}
	return (false);
}

void	ClientRequest::parse_chunked_body(Request& request)
{
	while (status == READING_CHUNKED)
	{
		if (crlf_received)
		{
			if (is_crlf_correct(0) == 1)
				is_keep_alive(request);
			return ;
		}

		if (!reading_data_chunked)
		{
			long size = get_chunk_size(extract_chunk_size());

			if (size < 0)
				return ;

			if (size == 0)
			{
				crlf_received = true;
				continue ;
			}

			current_data_size = static_cast<size_t>(size);
			current_data_size_readed = 0;
			reading_data_chunked = true;
		}

		if (reading_data_chunked)
		{
			if (data.size() < current_data_size + 2)
				return ;

			request.body.insert(
				request.body.end(),
				data.begin(),
				data.begin() + current_data_size
			);

			data.erase(0, current_data_size);

			if (is_crlf_correct(1) <= 0)
				return ;
		}
		if (check_the_size(request))
		{
			this->http_error_code = 413;
		}
	}
}

void	ClientRequest::parse_chunk(const char *buffer, size_t bytes_read, Request& request, const ServerConfig* active_config)
{
	config = active_config;

	if (buffer && bytes_read > 0)
		data.append(buffer, bytes_read);

	while (!data.empty() && status != COMPLETE && status != ERROR)
	{
		if (status == READING_REQUEST)
			parse_request_line(request);
		if (status == READING_HEADER)
			parse_headers(request);
		if (status == READING_BODY)
			parse_body(request);
		if (status == READING_CHUNKED)
		{
			parse_chunked_body(request);
			if (status == READING_CHUNKED)
				break ;
		}

		if (status == READING_REQUEST)
			break ;
		if (status == READING_HEADER && data.find("\r\n\r\n") == std::string::npos)
			break ;
		if (status == READING_BODY && data.empty())
			break ;
		if (status == READING_CHUNKED && data.empty())
			break ;
	}
}

void				ClientRequest::set_error_code(int code_error)
{
	http_error_code = code_error;
}

int					ClientRequest::get_error_code() const
{
	return (http_error_code);
}

RequestState	ClientRequest::get_status() const
{
	return (status);
}

int	ClientRequest::content_size(const std::string& str)
{
	char	*end;
	long	l;

	if (str.length() > 10 || (str.length() == 10 && str > "2147483647"))
	{
		status = ERROR;
		set_error_code(413);
		return (-1);
	}

	l = strtol(str.c_str(), &end, 10);
	return (static_cast<int>(l));
}

int ClientRequest::is_valid_size(const std::string& str, int code)
{
	if (str.empty())
	{
		status = ERROR;
		set_error_code(400);
		return (-1);
	}
	if (code == 0)
	{
		for (size_t i = 0; i < str.size(); ++i)
		{
			if (!std::isdigit(static_cast<unsigned char>(str[i])))
			{
				status = ERROR;
				set_error_code(400);
				return (-1);
			}
		}
	}
	else if (code == 1)
	{
		if (str != "chunked")
		{
			status = ERROR;
			set_error_code(501);
			return (-1);
		}
		return (1);
	}
	return (content_size(str));
}

void ClientRequest::parse_headers(Request& request)
{
	size_t	end_of_line;

	while ((end_of_line = data.find("\r\n")) != std::string::npos)
	{
		std::string	line = data.substr(0, end_of_line);
		data.erase(0, end_of_line + 2);

		if (line.empty())
		{	
			bool has_content_length = request.header.find("Content-Length") != request.header.end();
			bool has_transfer_encoding = request.header.find("Transfer-Encoding") != request.header.end();
			
			if (has_content_length && has_transfer_encoding)
			{
				set_error_code(400);
				status = ERROR;
				return ;
			}
			if (has_content_length)
			{
				int	size = is_valid_size(request.header["Content-Length"], 0);
				if (size < 0)
					return ;
				
				content_length = static_cast<size_t>(size);
				if (content_length == 0)
					status = COMPLETE;
				else
					status = READING_BODY;
				return ;
			}
			if (has_transfer_encoding)
			{
				if (is_valid_size(request.header["Transfer-Encoding"], 1) < 0)
					return;
				status = READING_CHUNKED;
				return ;
			}
			status = COMPLETE;
			return ;
		}
		size_t	key_pos = line.find(':');
		if (key_pos != std::string::npos && key_pos != 0)
		{
			std::string key = line.substr(0, key_pos);
			std::string value = line.substr(key_pos + 1);

			while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
				value.erase(0, 1);

			request.header[key] = value;
			if (key == "Cookie")
			{
				parse_cookies(request, value);
			}
		}
		else
		{
			status = ERROR;
			set_error_code(400);
			return ;
		}
	}
}