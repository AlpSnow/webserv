/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 17:37:16 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/17 17:46:25 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SessionManager.hpp"
#include <sstream>

SessionManager::SessionManager() : _counter(0)
{
}

bool SessionManager::hasSession(const std::string& id) const
{
	if (_sessions.find(id) != _sessions.end())
		return (true);
	return (false);
}

Session& SessionManager::getSession(const std::string& id)
{
	std::map<std::string, Session>::iterator it;

	it = _sessions.find(id);
	it->second.last_activity = time(NULL);
	return (it->second);
}

std::string SessionManager::generateSession()
{
	std::stringstream ss;

	_counter++;
	ss << time(NULL) << "_" << _counter;
	
	Session session;

	session.id = ss.str();
	session.created_at = time(NULL);
	session.last_activity = time(NULL);
	_sessions[session.id] = session;
	
	return (session.id);
}
