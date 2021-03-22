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
#include "schema.hpp"

#ifdef _DEBUG
	#include <iostream>
#endif

using namespace std;
using namespace boost;

/*
 * Constructor
 */
schema::schema()
{
	// nothing yet
}

/*
 * Destructor
 */
schema::~schema()
{
	// nothing yet
}

/*
 * Load an XML schema file
 * 
 * @param filename location of the schema file
 * @return true on successful loading, false otherwise.
 */
bool schema::load (string filename) {
	const xmlChar *xpathExpr = (xmlChar *)"//schema";
	xmlDocPtr doc;
    xmlXPathContextPtr xpathCtx; 
    xmlXPathObjectPtr xpathObj;

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
void schema::parseNodeset (xmlNodeSetPtr nodes) {
    xmlNodePtr cur, tmpNode;
    int size;
    int i;
    int elements;
    
    string strName;
    struct logFormat tmpFormat; 
    
    size = (nodes) ? nodes->nodeNr : 0;
    for(i = 0; i < size; ++i) {
	    cur = nodes->nodeTab[i];
	    
	    for (tmpNode = cur->children; tmpNode; tmpNode = tmpNode->next) {
	    	if (tmpNode->type == XML_ELEMENT_NODE) {
	    		// Schema name
	    		if (strncmp ((const char *)tmpNode->name, "name", strlen ("name")) == 0) {
	    			strName = (const char *)tmpNode->children->content;
	    			elements++;
	    		}
	    		// Schema format regex
	    		else if (strncmp ((const char *)tmpNode->name, "regex", strlen ("regex")) == 0) {
	    			try {
	    				tmpFormat.re.assign ((const char *)tmpNode->children->content);
	    			}
	    			catch (regex_error &e) {
	    				#ifdef _DEBUG
	    					cerr << e.what () << endl;
	    				#endif
	    				continue;
	    			}
	    			elements++;
	    		}
	    		else if (strncmp ((const char *)tmpNode->name, "columns", strlen ("columns")) == 0) {
	    			split ((const char *)tmpNode->children->content, ',', &(tmpFormat.columns));
	    			//  Trim all entries
	    			for (unsigned int i = 0; i < tmpFormat.columns.size(); i++)
	    				tmpFormat.columns[i] = trim (tmpFormat.columns[i]);
	    			elements++;
	    		}
	    		//cout << "Sub-element: '" << tmpNode->name << "'\n";
	    		//cout << tmpNode->children->content;
	    	}
	    }
	    m_mSchemas[strName] = tmpFormat;
    }
}

/*
 * Return a std::map containing the schemas.
 * 
 * @return schemas map containing the schema name (key) and the format (value)
 */
map<string, logFormat> & schema::schemas () {
	return m_mSchemas;
}
