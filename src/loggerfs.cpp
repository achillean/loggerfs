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
#include "loggerfs.hpp"

using namespace std;

#ifdef WITH_POSTGRESQL
	using namespace pqxx;
#endif

// In case the user got by the configure script, don't allow loggerfs
// to be compiled w/out database support.
#if !defined(WITH_POSTGRESQL) && !defined(WITH_MYSQL)
	#error "You must enable either PostgreSQL or MySQL database support for loggerfs. Please re-./configure loggerfs using --with-postgresql and/or --with-mysql"
#endif

#ifndef PREFIX
	#define PREFIX "/usr/local"
#endif

/*
 * Constructor
 */
loggerfs::loggerfs () {
	// Get the hostname of the current machine
	char hostname[128];
	gethostname (hostname, 128);
	m_strHostname = hostname;
}

/*
 * Destructor
 */
loggerfs::~loggerfs () {
	// Clear the connection cache
	#ifdef WITH_POSTGRESQL
		for (map<string, connection *>::iterator i = m_mPgsqlCache.begin ();
				i != m_mPgsqlCache.end (); ++i)
			delete i->second;
	#endif
	
	#ifdef WITH_MYSQL
		for(map<string, MYSQL *>::iterator i = m_mMysqlCache.begin ();
				i != m_mMysqlCache.end (); ++i) {
			mysql_close (i->second);
			delete i->second;
		}
	#endif
}

/*
 * Initialize the loggerfs class.
 * 
 * Loads the schemas.xml and logs.xml files, does
 * some validation to make sure the regex etc. is valid and
 * then creates the tables to hold the data (if they're not already created).
 * 
 * TODO:
 * - make sure that the column names don't conflict w/ SQL syntax (reserved words)
 * 
 * @return true on success, false otherwise
 */
bool loggerfs::initialize () {
	// Get the path to the installed configuration files
	string strPath (PREFIX);
	
	// Load the schemas.xml file
	schema Schema;
	if (!Schema.load ("/etc/loggerfs/schemas.xml")
			&& !Schema.load (strPath + "/etc/loggerfs/schemas.xml")
			&& !Schema.load ("schemas.xml")) {
		cout << "Error: Couldn't load the schemas file\n";
		return false;
	}
	m_mSchemas = Schema.schemas ();
	
	// Load the logs.xml configuration file
	config Config;
	if (!Config.load ("/etc/loggerfs/logs.xml")
			&& !Config.load (strPath + "/etc/loggerfs/logs.xml")
			&& !Config.load ("logs.xml")) {
		cout << "Error: Couldn't load the logs configuration file\n";
		return false;
	}
	m_mLogs = Config.logs ();
	
	// Create the tables
	if (!this->formatTables (m_mSchemas, m_mLogs)) {
		cerr << "Aborting: Couldn't initialize database tables\n";
		return false;
	}
	
	return true;
}

#ifdef _DEBUG
	/*
 	 * Debug helper function
 	 * 
 	 * @param args message to output
 	 */
	void loggerfs::debug (string args) {
		ofstream fout ("/tmp/debug.log", fstream::app);
		if (fout.bad() || !fout.is_open ())
			return;
		fout << args;
		fout.close ();
	}
	
	void loggerfs::debug (const char *args) {
		ofstream fout ("/tmp/debug.log", fstream::app);
		if (fout.bad () || !fout.is_open())
			return;
		fout << args;
		fout.close ();
	}
#endif

/**
 * Fuse: getattr
 * 
 * @param path location of the file
 * @param stbuf pointer to the stat struct
 * @return 0 on success, -ENOENT on failure
 */
int loggerfs::getattr(const char *path, struct stat *stbuf) {
    int res = 0;
    string strPath = path + 1;
    map<string, logInfo>::iterator iter = self->m_mLogs.find (strPath);

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0555; // read + execute
        stbuf->st_nlink = 2;
    }
    else if (iter != self->m_mLogs.end ()) {
    	// Special permissions were defined in the logs file
    	if (iter->second.permissions > 0)
    		stbuf->st_mode = S_IFREG | iter->second.permissions;
    	// Default File permissions
    	else
    		stbuf->st_mode = S_IFREG | 0222; // write only
    	stbuf->st_nlink = 1;
    	stbuf->st_size = 0;
    	stbuf->st_uid = iter->second.uid;
    	stbuf->st_gid = iter->second.gid;
    }
    // .refresh is a special file that allows reloading of configs/ schemas
    else if (strcmp (path, "/.refresh") == 0) {
    	stbuf->st_mode = S_IFREG | 0000;
    	stbuf->st_nlink = 1;
    	stbuf->st_size = 0;
    }
    else
        res = -ENOENT;

    return res;
}

int loggerfs::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // List all the log files
    for (map<string, logInfo>::iterator iter = self->m_mLogs.begin ();
    		iter != self->m_mLogs.end (); iter++)
    	filler (buf, iter->first.c_str(), NULL, 0);

    return 0;
}

int loggerfs::open(const char *path, struct fuse_file_info *fi) {
	/*
	 * Special loggerfs files
	 */
	// .refresh -> used to reload logs.xml and schemas.xml
	if (strncmp (path, "/.refresh", strlen ("/.refresh")) == 0) {
		string strPath (PREFIX);

		// Load the schemas
		schema Schema;
		if (!Schema.load (strPath + "/etc/loggerfs/schemas.xml")
				&& !Schema.load ("/etc/loggerfs/schemas.xml")
				&& !Schema.load ("schemas.xml")) {
			cout << "Error: Couldn't load the schemas file\n";
			return 0;
		}
		#ifdef _DEBUG
			self->debug ("Reloaded schemas.xml\n");
		#endif
		
		// Load the logs.xml configuration file
		config Config;
		if (!Config.load (strPath + "/etc/loggerfs/logs.xml")
				&& !Config.load ("/etc/loggerfs/logs.xml")
				&& !Config.load ("logs.xml")) {
			cout << "Error: Couldn't load the logs configuration file\n";
			return 0;
		}
		#ifdef _DEBUG
			self->debug ("Reloaded logs.xml\n");
		#endif
		
		// Create the tables
		if (!self->formatTables (Schema.schemas(), Config.logs()))
			return 0;
		#ifdef _DEBUG
			self->debug ("Reloaded tables\n");
		#endif
		
		// If both were successfully loaded then start actually using them
		self->m_mSchemas = Schema.schemas ();
		self->m_mLogs = Config.logs ();
		
		return 0;
	}
	
    // Make sure the path exists
    string strPath = path + 1;
    map<string, logInfo>::iterator iter = self->m_mLogs.find (strPath);
    if (iter == self->m_mLogs.end ())
    	return -ENOENT;

    return 0;
}

// Doesn't read anything from the database (yet)
int loggerfs::read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    return -EACCES; // don't allow reading of files
}

int loggerfs::write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    // Make sure the log file exists
    string strPath = path + 1;
    map<string, logInfo>::iterator lookup = self->m_mLogs.find (strPath);
    if (lookup == self->m_mLogs.end ())
    	return -ENOENT;
     
    // Convert the buffer into a string
    string strBuffer = buf;
    strBuffer = strBuffer.substr (0, size);
    
    vector<string> vLines = split (strBuffer, '\n');
    // Loop through all the lines
    string strLine = "";
    for (unsigned int i = 0; i < vLines.size(); ++i) {
	    // Trim the string
	    strLine = trim (vLines[i]);
	    
	    logInfo &info = self->m_mLogs[strPath];
	    boost::cmatch matches;
	    // Compare the regex against all schemas assigned to the log file
	    for (vector<string>::iterator s = info.schemas.begin ();
	    		s != info.schemas.end (); s++) {
	    	// Get the regular expression and column names
	    	logFormat &format = self->m_mSchemas[*s];
	    	
	    	// The current line matches the regex
	    	if (boost::regex_match (strLine.c_str(), matches, format.re)) {
	    		// The number of matches doesn't equal the number of columns in the table
	    		if (format.columns.size () != matches.size() - 1)
	    			continue;
	    		
	    		// Insert the line into the database
	    		self->sqlInsertLogLine (strPath, info, format, matches);
	    	}
	    }
    }
    
    return strBuffer.length ();
}

/************************************************
 * Private Methods
 ************************************************/
bool loggerfs::formatTables (map<string, struct logFormat> &mSchemas,
		map<string, struct logInfo> &mLogs) {
	unsigned int j = 0, k =0;
	
	// Loop through all the log files
	for (map<string, logInfo>::iterator iter = mLogs.begin ();
			iter != mLogs.end (); iter++) {
		vector<string> vSchemas;
		map<string, int> mColumns;
		
		// Create the connection caches
		#ifdef WITH_POSTGRESQL
			if (this->isPostgresql (iter->second.software)) {
				try {
					cerr << "Adding cache for " << iter->first << endl;
					m_mPgsqlCache[iter->first] = 
							new connection (this->pgConnectionString (iter->second));
				}
				catch (...) { }
			}
		#endif
		
		#ifdef WITH_MYSQL
			if (this->isMysql (iter->second.software)) {
				try {
					MYSQL *con = new MYSQL;
					// Initialize the connection
					mysql_init (con);
					if (!mysql_real_connect (con, iter->second.server.c_str(),
							iter->second.username.c_str(), iter->second.password.c_str(),
							iter->second.database.c_str(), 0, NULL, 0)) {
						cerr << "Error: Couldn't create a MySQL connection to " << iter->second.database.c_str() << endl;
						return false;
					}
					m_mMysqlCache[iter->first] = con;
				}
				catch (...) { }
			}
		#endif
		
		// Don't do anything if the table already exists
		if (this->tableExists (iter->second)) {
			#ifdef _DEBUG
				self->debug ("Table already exists: ");
				self->debug (iter->second.table);
				self->debug ("\n");
			#endif
			continue;
		}
		
		// Get a list of all columns by going through all schemas for the log file
		vSchemas = iter->second.schemas;
		for (j = 0; j < vSchemas.size (); j++) {
			#ifdef _DEBUG
				self->debug ("Parsing schema: ");
				self->debug (vSchemas[j]);
				self->debug ("\n");
			#endif
			// Skip the entry if the schema isn't defined
			if (mSchemas.find (vSchemas[j]) == mSchemas.end ()) {
				cerr << "Warning: Schema doesn't exist: " << vSchemas[j] << endl;
				continue;
			}
			// Add all columns
			for (k = 0; k < mSchemas[vSchemas[j]].columns.size (); k++)
				mColumns[mSchemas[vSchemas[j]].columns[k]] = 1;
		}
		
		// Create the table
		if (!this->createTable (iter->second, mColumns)) {
				cerr << "Error: Couldn't create the table " << iter->second.table
					 << " , please check the permissions\n";
			return false;
		}
	}
	
	#ifdef _DEBUG
		self->debug ("Exiting formatTables\n");
	#endif
	return true;
}

/*
 * Check if a table exists by trying to fetch zero rows (LIMIT 0).
 * 
 * @param info logInfo struct containing database information
 * @param strTable name of the table to check
 * 
 * @return true if the table exists, false otherwise
 */
bool loggerfs::tableExists (const logInfo &info) {
	#ifdef WITH_POSTGRESQL
		if (this->isPostgresql (info.software)) {
			try {
				connection con (this->pgConnectionString (info));
				work query (con);
				
				ostringstream strTmp;
				strTmp.str ("");
				strTmp	<< "SELECT * FROM " << info.table << " LIMIT 0;";
				query.exec (strTmp.str ());
				
				// Don't care how many rows returned, all that matters is that the
				// query completed successfully.
				return true;
			}
			catch (...) {
				return false;
			}
		}
	#endif
	
	#ifdef WITH_MYSQL
		if (this->isMysql (info.software)) {
			MYSQL con;
			
			// Initialize the connection
			mysql_init (&con);
			if (!mysql_real_connect (&con, info.server.c_str(),
					info.username.c_str(), info.password.c_str(),
					info.database.c_str(), 0, NULL, 0))
				return false;
			
			char *cstrTable = NULL;
			try {
				cstrTable = new char[info.table.length() * 2 + 1];
				mysql_real_escape_string (&con, cstrTable, info.table.c_str(),
						info.table.length());
			}
			catch (...) {
				return false;
			}
			ostringstream strQuery;
			strQuery << "SELECT * FROM " << cstrTable << " LIMIT 0;";
			if (mysql_query (&con, strQuery.str().c_str()))
				return false;
			
			mysql_close (&con);
			
			delete[] cstrTable;
			
			return true;
		}
	#endif
	
	return false;
}

bool loggerfs::createTable (const logInfo &info, const map<string, int> &mColumns) {
	// Create the SQL query
	string strQuery = sqlCreateTable (info, mColumns);
		
	#ifdef WITH_POSTGRESQL
		if (this->isPostgresql (info.software)) {
			try {
				connection con (this->pgConnectionString (info));
				work query (con);
				
				query.exec (strQuery);
				query.commit ();
				
				return true;
			}
			catch (...) {
				return false;
			}
		}
	#endif
	
	#ifdef WITH_MYSQL
		if (this->isMysql (info.software)) {
			MYSQL con;
			
			// Initialize the connection
			mysql_init (&con);
			if (!mysql_real_connect (&con, info.server.c_str(),
					info.username.c_str(), info.password.c_str(),
					info.database.c_str(), info.port, NULL, 0))
				return false;
			
			if (mysql_query (&con, strQuery.c_str()))
				return false;
			mysql_close (&con);
			
			return true;
		}
	#endif
	
	return false;
}
	
string loggerfs::sqlCreateTable (const logInfo &info, const map<string, int> &mColumns) {
	string strTmp;
	string strIdColumn = "";
	
	#ifdef WITH_POSTGRESQL
		if (this->isPostgresql (info.software)) {
			strIdColumn = "id serial not null primary key,";
		}
	#endif
	#ifdef WITH_MYSQL
		if (this->isMysql (info.software)) {
			strIdColumn = "id bigint not null primary key unique auto_increment,";
		}
	#endif
	
	ostringstream strQuery;
	strQuery << "CREATE TABLE " << info.table << "(" << strIdColumn
			 << "timestamp timestamp default now(),";
	for (map<string, int>::const_iterator iter = mColumns.begin ();
			iter != mColumns.end (); iter++)
		strQuery << iter->first << " text DEFAULT '' NOT NULL,";
	
	strTmp = strQuery.str ();
	strTmp.erase (strTmp.end() - 1);
	
	strTmp += ");";
	
	return strTmp;
}

bool loggerfs::sqlInsertLogLine (const string &strPath, const logInfo &info,
		const logFormat &format, const boost::cmatch &matches) {
	#ifdef _DEBUG
		self->debug ("Calling sqlInsertLogLine: ");
		self->debug (info.software);
		self->debug ("\n");
	#endif				
	#ifdef WITH_POSTGRESQL
		if (this->isPostgresql (info.software)) {
			try {
//				connection con (this->pgConnectionString (info));
				work query (*m_mPgsqlCache[strPath]);
				/*
				 *  Build the query string
				 */
				string strQuery = "";
				strQuery += "INSERT INTO " + info.table + "(";
				for (vector<string>::const_iterator iter = format.columns.begin ();
						iter != format.columns.end (); iter++)
					strQuery += *iter + ",";
				
				strQuery.erase (strQuery.end() - 1); // remove the last ','
				strQuery += ") VALUES (";
				
				// Skip the 0-th match because that just contains the original string
				for (boost::cmatch::const_iterator iter = matches.begin () + 1;
						iter != matches.end (); iter++) {
					string strMatch (iter->first, iter->second);
					strQuery += "'" + query.esc (strMatch) + "',";
				}
				
				strQuery.erase (strQuery.end() - 1); // remove the last ','
				strQuery += ");";
				
				query.exec (strQuery);
				query.commit ();
				
				return true;
			}
			catch (...) {
				return false;
			}
		}
	#endif
	
	#ifdef WITH_MYSQL
		if (this->isMysql (info.software)) {
			MYSQL con = *m_mMysqlCache[strPath];
			
			#ifdef _DEBUG
				self->debug ("Connecting to mysql server\n");
			#endif
			// Initialize the connection
//			mysql_init (&con);
//			if (!mysql_real_connect (&con, info.server.c_str(),
//					info.username.c_str(), info.password.c_str(),
//					info.database.c_str(), 0, NULL, 0))
//				return false;
			
			/*
			 *  Build the query string
			 */
			string strQuery = "";
			strQuery += "INSERT INTO " + info.table + "(";
			for (vector<string>::const_iterator iter = format.columns.begin ();
					iter != format.columns.end (); iter++)
				strQuery += *iter + ",";
			
			strQuery.erase (strQuery.end() - 1); // remove the last ','
			strQuery += ") VALUES (";
			
			char *cstrMatch = NULL;
			// Skip the 0-th match because that just contains the original string
			for (boost::cmatch::const_iterator iter = matches.begin () + 1;
					iter != matches.end (); iter++) {
				string strMatch (iter->first, iter->second);
				// I'm using mysql_real_escape_string because it considers the character
				// set, which mysql_escape_string doesn't. But it doesn't work elegantly
				// w/ the STL string class.
				try {
					cstrMatch = new char[strMatch.length() * 2 + 1];
					mysql_real_escape_string (&con, cstrMatch, strMatch.c_str(),
							strMatch.length());
				
					strQuery += "'";
					strQuery += cstrMatch;
					strQuery += "',";
					delete[] cstrMatch;
				}
				catch (...) {
					return false;
				}
			}
			
			strQuery.erase (strQuery.end() - 1); // remove the last ','
			strQuery += ");";
			
			#ifdef _DEBUG
				self->debug (strQuery);
			#endif
			
			// execute the query
			if (mysql_query (&con, strQuery.c_str ()))
				return false;
			
//			mysql_close (&con);
		}
	#endif
	
	return false;
}

#ifdef WITH_POSTGRESQL
	/*
 	* PostgreSQL Helper functions
 	*/
	bool loggerfs::isPostgresql (const string &args) {
		return (args == "postgresql" || args == "pgsql") ? true : false;
	}
	
	/*
	 * Creates a PostgreSQL connection string based on the logInfo struct.
	 * 
	 * @param info the logInfo struct
	 * 
	 * @return string containing the postgresql connection string
	 */
	string loggerfs::pgConnectionString (const logInfo &info) {
		ostringstream strTmp;
		strTmp	<< "dbname=" << info.database
				<< " user=" << info.username
				<< " password=" << info.password
				<< " host=" << info.server;
		if (info.port > 0)
			strTmp << " port=" << info.port;
		cerr << "Connection string: " << strTmp.str() << endl;
		return strTmp.str ();
	}
#endif

#ifdef WITH_MYSQL
	/*
 	* MySQL Helper functions
 	*/
	bool loggerfs::isMysql (const string &args) {
		return args == "mysql" ? true : false;
	}
#endif
