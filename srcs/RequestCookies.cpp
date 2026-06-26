/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestCookies.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlavry <mlavry@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 12:57:18 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/11 12:57:19 by mlavry           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "RequestCookies.hpp"
#include <cstddef>

static std::string	trim(const std::string& str)
{
	size_t start = 0;
	size_t end = str.length();

	while (start < end && (str[start] == ' '  || str[start] == '\t'))
		start++;
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t'))
		end--;
	return (str.substr(start, end - start));
}

static void add_cookie(Request& request, const std::string& part)
{
	size_t equal = part.find("=");

	if (equal == std::string::npos)
		return ;
	std::string name = trim(part.substr(0, equal));
	std::string value = trim(part.substr(equal + 1));
	if (!name.empty())
		request.cookies[name] = value;
}

void parse_cookies(Request& request, const std::string& cookie_header)
{
	size_t start = 0;

	while (start < cookie_header.length())
	{
		size_t end = cookie_header.find(";", start);
		if (end == std::string::npos)
			end = cookie_header.length();
		add_cookie(request, trim(cookie_header.substr(start, end - start)));
		start = end + 1;
	}
}
