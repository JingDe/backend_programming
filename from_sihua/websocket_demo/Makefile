
wsclienttest:=wsclienttest
wsservertest:=wsservertest

CXX=g++
CXXFLAGS=-g -Wall -std=c++0x
LDFLAGS=-rdynamic -l



.PHONY: all
all: $(wsclienttest) #$(wsservertest)

$(wsclienttest): $(wsclienttest_obj)
	$(CXX) -o $@ $^


.PHONY: clean
clean:
	-rm $(wsclienttest) $(wsclienttest_obj)
	-rm $(wsservertest)


