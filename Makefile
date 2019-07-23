
CC = gcc
CXX = g++

SRCS = c-logger.c test.c
HDRS = c-logger.h
OBJS = $(SRCS:.c=.o)

C_PROG = c-logger
CXX_PROG = cxx-logger

C_FLAGS = -Werror -g 
CXX_FLAGS = -Werror -g 
LD_FLAGS = -lpthread

all: $(C_PROG) $(CXX_PROG)

$(C_PROG): $(SRCS) $(HDRS)
	$(CC) -c $(C_FLAGS) $(SRCS)
	$(CC) -o $(C_PROG) $(OBJS) $(LD_FLAGS) 

$(CXX_PROG): $(SRCS) $(HDRS)
	$(CXX) -c $(CXX_FLAGS) $(SRCS)
	$(CXX) -o $(CXX_PROG) $(OBJS) $(LD_FLAGS)

clean:
	$(RM) $(C_PROG) $(CXX_PROG) $(OBJS)
