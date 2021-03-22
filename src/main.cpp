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

#include <iostream>

void printUsage ();

int main (int argc, char **argv) {
	// Show usage if no args are given
	if (argc < 2) {
		printUsage ();
		return 1;
	}
	loggerfs LogFS;
	if (!LogFS.initialize ())
		return -1;
	return loggerfs::main (argc, argv, NULL, &LogFS);
}

void printUsage () {
	std::cout 	<< "loggerfs by achillean\n"
				<< "Usage: loggerfs [fuse options] <mount point>\n"
				<< "Example: loggerfs -o allow_other /var/loggerfs\n";
}
