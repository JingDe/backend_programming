
SOURCES = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SOURCES))
TARGETS = $(patsubst %.cpp, %, $(SOURCES))

CXX = g++

INCLUDES = -I libevent-2.1.11-stable/include/
CFLAGS = -Wall -g -c


LIB = -levent
LIB_PATH = libevent-2.1.11-stable/.libs/
LDFLAGS = -L $(LIB_PATH) -Wl,-rpath=$(LIB_PATH) 


.PHONY : all
all : $(TARGETS)

$(TARGETS): %:%.o
	$(CXX) $< -o $@ $(LIB) $(LDFLAGS)


%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@ $(INCLUDES)


.PHONY : clean
clean :
	-rm -rf $(TARGETS) $(OBJS)
	
.PHONY : test
test :
	@echo $(SOURCES)
	echo $(OBJS)
	@echo $(TARGETS)