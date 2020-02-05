
TEST_REDIS_CLIENT_SRC=$(wildcard ./*.cpp)
TEST_REDIS_CLIENT_OBJ=$(patsubst %.cpp, %.o, $(TEST_REDIS_CLIENT_SRC))

TEST_REDIS_CLIENT_TARGET=test_redisclient



.PHONY: all
all: $(TEST_REDIS_CLIENT_TARGET)

$(TEST_REDIS_CLIENT_TARGET):
	@echo `pwd`
	g++ -g -Wall -o $(TEST_REDIS_CLIENT_TARGET) $(TEST_REDIS_CLIENT_SRC) -I . -I log4cxx -Wl,-rpath=lib:/usr/local/lib -L lib -L /usr/local/lib -llog4cxx -lglog -std=c++0x



.PHONY: clean
clean:
	rm $(TEST_REDIS_CLIENT_TARGET)
	rm $(TEST_REDIS_CLIENT_OBJ)

