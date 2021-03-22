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
 
#ifndef SCHEMA_HPP_
#define SCHEMA_HPP_

#include <string>
#include <vector>
#include <map>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <boost/regex.hpp>

#include "general.hpp"

struct logFormat {
	boost::regex re;
	std::vector<std::string> columns;
};

class schema
{
	/*
	 * Public methods
	 */
	public:
		schema(); // constructor
		~schema(); // destructor
		
		bool load (std::string); // Load a schema file
		std::map<std::string, logFormat> & schemas ();
		
	/*
	 * Private methods
	 */
	private:
		void parseNodeset(xmlNodeSetPtr nodes);
	
	/*
	 * Private variables
	 */
	private:
		std::map<std::string, logFormat> m_mSchemas;
};

#endif /*SCHEMA_HPP_*/
