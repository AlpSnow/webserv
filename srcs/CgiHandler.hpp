/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/30 18:27:50 by cnamoune          #+#    #+#             */
/*   Updated: 2026/06/15 17:43:56 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

#include "Response.hpp"

enum	CGI_STATE
{
	STAND_BY,
	CGI_INIT,
	CGI_WRITING,
	CGI_READING,
	CGI_FINISHED,
	CGI_ERROR
};

class   CgiHandler
{
	public :
	
		CgiHandler();
		~CgiHandler();

        int	handle_script(
            const Request& request,
            const std::string& cgi_extention,
			const std::string& executable,
            const std::string& target_file_path,
            PathInfo path_info
        );

        int		http_error_code;
		
		void	create_env_variable(const Request& request, const std::string& target_file_path);
		void	print_env_variable() const;

        bool	prepare_script(const std::string& cgi_extention, const std::string& target_file_path,
							PathInfo target_path_info);

        bool	cgi_exist(const std::string& cgi_extention, const std::string& target_file_path,
						PathInfo target_path_info);

		pid_t						cgi_pid;
		int							pipe_in;
		int							pipe_out;
		
		CGI_STATE					stats;
		
		std::vector<std::string>	env_variable;
		char						**envp;
		std::vector<char>			body_to_sends;
		std::vector<char>			response_buffer;

		size_t						bytes_sended;
					
		time_t						start_time;
};

#endif