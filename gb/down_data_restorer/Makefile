
TEST_REDIS_CLIENT_SRC=$(wildcard ./*.cpp)
TEST_REDIS_CLIENT_OBJ=$(patsubst %.cpp, %.o, $(TEST_REDIS_CLIENT_SRC))

SRC_OUT_OF_DDR=../../gb_down_linker/down_gb_server/executing_invite_cmd.cpp ../../gb_down_linker/down_gb_server/channel.cpp ../../gb_down_linker/down_gb_server/device.cpp ../../gb_down_linker/down_gb_server/channel_list.cpp ../../base_library/time.cpp ../../base_library/log.cpp ../../base_library/logger.cpp

TEST_REDIS_CLIENT_TARGET=test_redisclient



.PHONY: all
all: $(TEST_REDIS_CLIENT_TARGET)

$(TEST_REDIS_CLIENT_TARGET):
	@echo `pwd`
	g++-8 -g -Wall -o $(TEST_REDIS_CLIENT_TARGET) $(SRC_OUT_OF_DDR) $(TEST_REDIS_CLIENT_SRC) -I ../../gb_down_linker/down_gb_server -I ../../ -I ./ -L ../../third_party/lib -lpthread -lglog -lrt -std=c++17



.PHONY: clean
clean:
	-rm downdatarestorerlogdir/*
	-rm test_redisclient.log
	-rm $(TEST_REDIS_CLIENT_TARGET)
	-rm $(TEST_REDIS_CLIENT_OBJ)

