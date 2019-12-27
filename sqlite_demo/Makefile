
OS_TEST=osTest
PAGER_TEST=pagerTest

OS_DEMO_DIR=os
PAGER_DEMO_DIR=pager

OS_DEMO_SRC=$(wildcard ./$(OS_DEMO_DIR)/*.c)
OS_API_SRC:=$(OS_DEMO_SRC)
OS_API_SRC:=$(filter-out test_os.c, $(OS_API_SRC))


PAGER_DEMO_SRC:=$(wildcard ./$(PAGER_DEMO_DIR)/*.c)
PAGER_DEMO_SRC:=$(filter-out pcache_origin.c, $(PAGER_DEMO_SRC))


OS_DEMO_OBJ=$(patsubst %.c, %.o, $(OS_DEMO_SRC))
OS_API_OBJ=$(patsubst %.c, %.o, $(OS_API_SRC))
# TODO 
OS_API_OBJ= os/os.o os/os_unix.o

PAGER_DEMO_OBJ=$(patsubst %.c, %.o, $(PAGER_DEMO_SRC))
ALL_OBJ= $(OS_DEMO_OBJ) $(PAGER_DEMO_OBJ)


.PHONY: all
all: $(OS_TEST) $(PAGER_TEST)


CC=gcc
GCC_FLAGS=-g -Wall
INCLUDE_COMMON=-I include/ -I os


$(OS_TEST): $(OS_DEMO_OBJ)
	$(CC) -o $(OS_TEST) $(OS_DEMO_OBJ)

#%.o:%.c
#	$(CC) $(GCC_FLAGS) $(INCLUDE_COMMON) -c $< -o $@ 

$(ALL_OBJ): %.o:%.c
	$(CC) $(GCC_FLAGS) $(INCLUDE_COMMON) -c $< -o $@ 

	
$(PAGER_TEST): $(PAGER_DEMO_OBJ) $(OS_API_OBJ)
	echo $(OS_DEMO_SRC)
	echo $(OS_API_SRC)
	echo $(PAGER_DEMO_SRC)
	$(CC) -o $@ $^


.PHONY: clean
clean:
	-rm -rf $(OS_TEST) $(PAGER_TEST) 
	-rm -rf $(OS_DEMO_OBJ) $(PAGER_DEMO_OBJ) *.o
