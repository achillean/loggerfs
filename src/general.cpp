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
#include "general.hpp"

using namespace std;

/*
 * Split a string into sub-strings.
 * 
 * @param s string to be split
 * @param c character that separates the strings
 * @return vector<string> containing the separated strings as elements
 */
vector<string> split (string s, char c, vector<string> *vReturn)
{
	vector<string> vTmp;
	string pattern = ""; pattern += c;
	int start = s.find (pattern);
	int end = 0;

	if (vReturn == NULL)
		vReturn = &vTmp;
	else if (vReturn != NULL)
		vReturn->clear ();
	
	if (start == (signed int)string::npos) {
		vReturn->push_back (s);
		return *vReturn;
	}
	vReturn->push_back (s.substr (0,start));

	start++;
	while ((end = s.find(pattern, start)) != (signed int)string::npos) {
		if (end == start) {
			start++;
			continue;
		}
		vReturn->push_back (s.substr(start, end - start));
		start = end + 1;
	}
	vReturn->push_back (s.substr(start));

	return *vReturn;
}

string trim(string s) {
	if(s.length() == 0)
		return s;
	
	int beg = s.find_first_not_of(" \a\b\f\n\r\t\v");
	int end = s.find_last_not_of(" \a\b\f\n\r\t\v");
	if(beg == (signed int)string::npos) // No non-spaces
		return "";
	  
	return s.substr(beg, end - beg + 1);
}

int strtoint (string s) {
	stringstream stream (s);
	int out;
	
	stream >> std::dec >> out;
	
	return out;
}

int strtooct (string s) {
	stringstream stream (s);
	int out;
	
	stream >> std::oct >> out;
	
	return out;
}

int strtoint (const char *s) {
	stringstream stream (s);
	int out;
	
	stream >> std::dec >> out;
	
	return out;
}

int strtooct (const char *s) {
	stringstream stream (s);
	int out;
	
	stream >> std::oct >> out;
	
	return out;
}

bool isnumber(const char *args) {
	char *ok;
	
	strtod (args, &ok);

	return !isspace(*args) && strlen(ok) == 0;
}
