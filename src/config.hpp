/*
 * loggerfs: a virtual file system to store logs in a database
 * Copyright (C) 2007 John C. Matherly jmath@itauth.com
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif
 
#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cctype>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include "general.hpp"

struct logInfo {
	std::string software;
	std::string server;
	std::string database;
	std::string table;
	std::string username;
	std::string password;
	int port;
	int uid;
	int gid;
	int permissions;
	std::vector<std::string> schemas;
};

class config {
	public:
		config();
		~config();
		
		bool load (std::string); // Load a schema file
		std::map<std::string, logInfo> & logs ();
		
	/*
	 * Private methods
	 */
	private:
		void parseNodeset(xmlNodeSetPtr nodes);
	
	/*
	 * Private variables
	 */
	private:
		std::map<std::string, logInfo> m_mLogs;
};

#endif /*CONFIG_HPP_*/
