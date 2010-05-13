constants: constants.cpp constants.h mongoose.o
	g++ -o constants constants.cpp mongoose.o -Wall -Wextra -O2 -g -ldl -lpthread

mongoose.o: mongoose.c mongoose.h
	gcc -D_POSIX_SOURCE -D_BSD_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -ldl -lpthread -std=c99 -pedantic -O2 -fomit-frame-pointer -DNO_CGI -DNO_SSL -DNO_SSI -shared -fPIC -fpic -s -c -o mongoose.o mongoose.c

