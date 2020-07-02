
header=$(wildcard *.h)
src=$(wildcard *.cpp)

CXX:=g++

libwebsocketclient.so: websocketclient.o
	$(CXX) -shared -o $@ $^

websocketclient.o: $(src) #$(header)
	$(CXX) -g -Wall -o $@ $^

$(src): $(header)
	
