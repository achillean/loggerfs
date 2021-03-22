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
 
#ifndef LOGGERFS_HPP_
#define LOGGERFS_HPP_

#include "fusexx.hpp"
#include "schema.hpp"
#include "config.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

// C-headers (required for uid lookup in passwd file)
#include <sys/types.h>
#include <pwd.h>

#ifdef WITH_POSTGRESQL
	#include <pqxx/pqxx>
#endif

#ifdef WITH_MYSQL
	#include <mysql.h>
#endif

class loggerfs : public fusexx::fuse<loggerfs> {
	public:
		// Constructor
		loggerfs ();
		// Destructor
		~loggerfs ();
		bool initialize ();
		
		#ifdef _DEBUG
			// Debug method
			void debug (std::string);
			void debug (const char *);
		#endif
		
		/*
		 * Static Fuse functions
		 */
		static int getattr (const char *, struct stat *);
		static int readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
		static int open (const char *, struct fuse_file_info *);
		static int read (const char *, char *, size_t, off_t, struct fuse_file_info *);
		static int write (const char *, const char *, size_t, off_t, struct fuse_file_info *);
	
	/*
	 * Private methods
	 */
	private:
		bool formatTables (std::map<std::string, struct logFormat> &,
				std::map<std::string, struct logInfo> &);
		bool tableExists (const logInfo &);
		bool createTable (const logInfo &, const std::map<std::string, int> &);
		
		#ifdef WITH_POSTGRESQL
			// PostgreSQL helper functions
			bool isPostgresql (const std::string &);
			std::string pgConnectionString (const logInfo &);
		#endif
		
		#ifdef WITH_MYSQL
			bool isMysql (const std::string &);
		#endif
		
		// SQL helper functions
		std::string sqlCreateTable (const logInfo &, const std::map<std::string, int> &);
		bool sqlInsertLogLine (const std::string &, const logInfo &, const logFormat &, const boost::cmatch &); 
		
	/*
	 * Private variables
	 */
	private:
		std::string m_strDatabase;
		std::string m_strHostname;
		std::map<std::string, struct logFormat> m_mSchemas;
		std::map<std::string, struct logInfo> m_mLogs;
		
		#ifdef WITH_POSTGRESQL
			std::map<std::string, pqxx::connection *> m_mPgsqlCache;
		#endif
		
		#ifdef WITH_MYSQL
			std::map<std::string, MYSQL *> m_mMysqlCache;
		#endif
};

#endif /*LOGGERFS_HPP_*/
