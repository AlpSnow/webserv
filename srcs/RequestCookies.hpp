/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestCookies.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/11 13:14:35 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/17 17:32:50 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTCOOKIES_HPP
# define REQUESTCOOKIES_HPP

#include "Request.hpp"
#include <string>

void parse_cookies(Request& request, const std::string& cookie_header);

#endif