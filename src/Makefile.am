bin_PROGRAMS = loggerfs
loggerfs_SOURCES = main.cpp \
		   loggerfs.cpp loggerfs.hpp \
		   config.cpp config.hpp \
		   schema.cpp schema.hpp \
		   general.cpp general.hpp \
		   fusexx.hpp
loggerfs_LDFLAGS = @LDFLAGS@
loggerfs_CPPFLAGS = @CPPFLAGS@ -Wall -O3 `pkg-config fuse --cflags` `xml2-config --cflags`
INCLUDES = -I/usr/local/include -I/usr/include
