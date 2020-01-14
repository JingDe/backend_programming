
TEST_REDIS_CLIENT_SRC=$(wildcard ./*.cpp)
TEST_REDIS_CLIENT_OBJ=$(patsubst %.cpp, %.o, $(TEST_REDIS_CLIENT_SRC))

TEST_REDIS_CLIENT_TARGET=test_redisclient

CC=gcc
CFLAGS=-g -Wall -c
INCLUDE=-I . -I log4cxx

LIBPATH=-L lib
LD_FLAGS= $(LIBPATH) -Wl,-rpath=$(LIBPATH)
LIBS=-llog4cxx

.PHONY: all
all: $(TEST_REDIS_CLIENT_TARGET)

all:
	g++ -g -Wall -o $(TEST_REDIS_CLIENT_TARGET) $(TEST_REDIS_CLIENT_SRC) -I . -I log4cxx -Wl,-rpath=lib -L lib -llog4cxx



.PHONY: clean
clean:
	rm $(TEST_REDIS_CLIENT_TARGET)
	rm $(TEST_REDIS_CLIENT_OBJ)

