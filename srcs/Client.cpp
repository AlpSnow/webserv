/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/21 13:07:46 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/08 14:06:14 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client() : fd(-1), bytesSent(0), isKeepAlive(true),
	hasStartTime(false), startTime(0), lastActivity(std::time(NULL)),
	statusCode(0)
{

}

Client::Client(int _fd) : fd(_fd), bytesSent(0), isKeepAlive(true),
	hasStartTime(false), startTime(0), lastActivity(std::time(NULL)),
	statusCode(0)
{
	
}
