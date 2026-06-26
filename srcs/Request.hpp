/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 14:43:22 by cnamoune          #+#    #+#             */
/*   Updated: 2026/06/17 17:34:23 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include "../config_loader/Config.hpp"

enum    RequestState
{
    READING_REQUEST,
    READING_HEADER,
    READING_BODY,
    READING_CHUNKED,
    COMPLETE,
	TIME_OUT,
    ERROR
};

struct Request
{
    Request();
	~Request();
	std::string                         method;
    std::string                         path;
	std::string							query;
    std::string                         http_version;
	std::string							get_host() const;

    std::map<std::string, std::string>  header;
	
	std::map<std::string, std::string>	cookies;

	// bool								is_a_file();
	bool								keep_alive;
	void								print_body() const;
	std::string							file_path;
	std::vector<char>	                body;

	const LocationConfig*				location_match;
	void								clear();
	
};

class ClientRequest
{
    private:
		RequestState	status;

		const ServerConfig*	config;

		std::string		data;
		size_t			content_length;
		size_t			body_bytes_readed;

		int				file_fd;
		std::string		file_name;

		void			parse_request_line(Request& request);
		void			parse_headers(Request& request);
		void			parse_body(Request& request);
		void			parse_chunked_body(Request& request);
		long			get_chunk_size(const std::string& hex_size);
		std::string		extract_chunk_size();
		int				is_valid_size(const std::string& str, int code);
		int				content_size(const std::string& str);
		int				is_crlf_correct(int code);
		bool			is_keep_alive(Request& request);
		
		int				http_error_code;
		size_t			current_data_size;
		size_t			current_data_size_readed;
		bool			reading_data_chunked;
		bool			crlf_received;
		bool			check_the_size(const Request& request);
		
	public:
        ClientRequest();
        ~ClientRequest();

		void			parse_chunk(const char *buffer, size_t bytes_read, Request& request, const ServerConfig* config);
		RequestState	get_status() const;
		
		int				get_error_code() const;
		void			set_error_code(int code_error);
		// void			clear();
};

#endif