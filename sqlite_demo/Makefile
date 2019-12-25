
OS_TEST=osTest
PAGER_TEST=pagerTest

OS_DEMO_DIR=os
PAGER_DEMO_DIR=pager

OS_DEMO_SRC=$(wildcard ./$(OS_DEMO_DIR)/*.c)
PAGER_DEMO_SRC=$(wildcard ./$(PAGER_DEMO_DIR)/*.c)

OS_DEMO_OBJ=$(patsubst %.c, %.o, $(OS_DEMO_SRC))
PAGER_DEMO_OBJ=$(patsubst %.c, %.o, $(PAGER_DEMO_SRC))


.PHONY: all
all: $(OS_TEST)
#all: $(OS_TEST) $(PAGER_TEST)


CC=gcc
GCC_FLAGS=-g -Wall
INCLUDE_COMMON=-I include/


# g++ -g -Wall -o os/osTest os/test_OS.c os/os.c os/os_unix.c -I os/ -I include

# g++ -g -Wall -o os/osTest os/test_OS.o os/os.o os/os_unix.o -I os/ -I include
# g++ 
$(OS_TEST): $(OS_DEMO_OBJ)
	$(CC) -o $(OS_TEST) $(OS_DEMO_OBJ)

%.o:%.c
	$(CC) $(GCC_FLAGS) $(INCLUDE_COMMON) -c $< -o $@

	
#$(PAGER_TEST):


.PHONY: clean
clean:
	-rm -rf $(OS_TEST) $(OS_DEMO_OBJ) *.o
