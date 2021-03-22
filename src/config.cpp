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
#include "config.hpp"

#ifdef _DEBUG
	#include <iostream>
#endif

using namespace std;

/*
 * Constructor
 */
config::config () {
	// nothing yet
}

/*
 * Destructor
 */
config::~config () {
	// nothing yet
}

/*
 * Load an XML log configuration file
 * 
 * @param filename location of the schema file
 * @return true on successful loading, false otherwise.
 */
bool config::load (string filename) {
	const xmlChar *xpathExpr = (xmlChar *)"//log";
	xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx; 
    xmlXPathObjectPtr xpathObj;

	cerr << "Loading " << filename << endl;
    /* Load XML document */
    doc = xmlParseFile(filename.c_str ());
    if (doc == NULL) {
    	#ifdef _DEBUG
			cerr << "Error: Unable to load schema file\n";
		#endif
		return false;
    }

    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
    	#ifdef _DEBUG
        	cerr << "Error: unable to create new XPath context\n";
        #endif
        xmlFreeDoc (doc); 
        return false;
    }

    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression (xpathExpr, xpathCtx);
    if(xpathObj == NULL) {
    	#ifdef _DEBUG
        	cerr << "Error: unable to evaluate xpath expression " <<  xpathExpr << endl;
        #endif
        xmlXPathFreeContext (xpathCtx); 
        xmlFreeDoc (doc); 
        return false;
    }

    /* Get the schema info */
    parseNodeset (xpathObj->nodesetval);

    /* Cleanup */
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx); 
    xmlFreeDoc(doc); 
    
    return true;
}

/**
 * Parse the xmlNodeSet and extract the relevant schema information.
 * 
 * @param nodes XML node set
 */
void config::parseNodeset (xmlNodeSetPtr nodes) {
    xmlNodePtr cur, tmpNode;
    int size;
    int i;
    
    string strLocation;//, strSoftware, strServer, strDatabase, strUsername, strPassword, strFormat;
    string strTmp;
    struct logInfo tmpInfo;
    
    size = (nodes) ? nodes->nodeNr : 0;
    for(i = 0; i < size; ++i) {
	    cur = nodes->nodeTab[i];
	    //cout << "Element '" << cur->name << "'\n";
	    
	    // Initialize the tmpInfo struct
	    tmpInfo.uid = tmpInfo.gid = tmpInfo.port = tmpInfo.permissions = 0;
	    
	    for (tmpNode = cur->children; tmpNode; tmpNode = tmpNode->next) {
	    	if (tmpNode->type == XML_ELEMENT_NODE) {
	    		// Log location
	    		if (strncmp ((const char *)tmpNode->name, "location",
	    				strlen ("location")) == 0) {
	    			strLocation = (const char *)tmpNode->children->content;
	    		}
	    		// Schemas
	    		else if (strncmp ((const char *)tmpNode->name, "schemas",
	    				strlen ("schemas")) == 0) {
	    			split ((const char *)tmpNode->children->content, ',', &(tmpInfo.schemas));
	    			//  Trim all entries
	    			for (unsigned int i = 0; i < tmpInfo.schemas.size(); i++)
	    				tmpInfo.schemas[i] = trim (tmpInfo.schemas[i]);
	    		}
	    		// Database software
	    		else if (strncmp ((const char *)tmpNode->name, "database-software",
	    				strlen ("database-software")) == 0) {
	    			tmpInfo.software = (const char *)tmpNode->children->content;
	    		}
	    		// Database
	    		else if (strncmp ((const char *)tmpNode->name, "database",
	    				strlen ("database")) == 0) {
	    			tmpInfo.database = (const char *)tmpNode->children->content;
	    		}
	    		// Table
	    		else if (strncmp ((const char *)tmpNode->name, "table",
	    				strlen ("table")) == 0) {
	    			tmpInfo.table = (const char *)tmpNode->children->content;
	    		}
	    		// Server
	    		else if (strncmp ((const char *)tmpNode->name, "server",
	    				strlen ("server")) == 0) {
	    			tmpInfo.server = (const char *)tmpNode->children->content;
	    		}
	    		// Username
	    		else if (strncmp ((const char *)tmpNode->name, "username",
	    				strlen ("username")) == 0) {
	    			tmpInfo.username = (const char *)tmpNode->children->content;
	    		}
	    		// Password
	    		else if (strncmp ((const char *)tmpNode->name, "password",
	    				strlen ("password")) == 0) {
	    			if (!tmpNode->children)
	    				tmpInfo.password = "";
	    			else
	    				tmpInfo.password = (const char *)tmpNode->children->content;
	    		}
	    		else if (strncmp ((const char *)tmpNode->name, "port",
	    				strlen ("port")) == 0) {
	    			tmpInfo.port = strtoint ((const char *)tmpNode->children->content);
	    		}
	    		// File Owner
	    		else if (strncmp ((const char *)tmpNode->name, "uid",
	    				strlen ("uid")) == 0) {
	    			// If the Log file contains a number then conver that directly
	    			if (isnumber ((const char *)tmpNode->children->content))
	    				tmpInfo.uid = strtoint ((const char *)tmpNode->children->content);
	    			// Otherwise try to look up if the string is a user and use that uid
	    			else {
	    				struct passwd *pwd = getpwnam ((const char *)tmpNode->children->content);
	    				if (pwd)
	    					tmpInfo.uid = pwd->pw_uid;
	    			}
	    		}
	    		// File Group
	    		else if (strncmp ((const char *)tmpNode->name, "gid",
	    				strlen ("gid")) == 0) {
	    			if (isnumber ((const char *)tmpNode->children->content))
	    				tmpInfo.gid = strtoint ((const char *)tmpNode->children->content);
	    			else {
	    				struct group *grp = getgrnam ((const char *)tmpNode->children->content);
	    				if (grp)
	    					tmpInfo.gid = grp->gr_gid;
	    			}
	    				
	    		}
	    		// Permissions
	    		else if (strncmp ((const char *)tmpNode->name, "permissions",
	    				strlen ("permissions")) == 0) {
	    			tmpInfo.permissions = strtooct ((const char *)tmpNode->children->content);
	    		}
	    	}
	    }
	    // Put the log file information into the map
	    m_mLogs[strLocation] = tmpInfo;
    }
}

/*
 * Return a std::map containing the schemas.
 * 
 * @return schemas map containing the schema name (key) and the format (value)
 */
map<string, logInfo> & config::logs () {
	return m_mLogs;
}
