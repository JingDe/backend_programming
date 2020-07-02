
liblog.so: log.o
	g++ -shared -o $@ $^

log.o: log.cpp
	g++ -g -Wall -std=c++0x -o $@ $^

