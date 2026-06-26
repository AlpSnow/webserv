/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cnamoune <cnamoune@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/15 17:37:05 by mlavry            #+#    #+#             */
/*   Updated: 2026/06/17 17:46:20 by cnamoune         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSIONMANAGER_HPP
# define SESSIONMANAGER_HPP

#include <string>
#include <map>
#include <ctime>

struct Session
{
	std::string id;
	std::map<std::string, std::string> data;
	time_t created_at;
	time_t last_activity;
};

class SessionManager
{
	public:
		SessionManager();
		
		bool hasSession(const std::string& id) const;
		Session& getSession(const std::string& id);
		std::string generateSession();
	private:
		std::map<std::string, Session> _sessions;
		unsigned int _counter;
};

#endif