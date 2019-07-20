
$(CC) = gcc

SRCS = c-logger.c test.c
HDRS = c-logger.h
OBJS = $(SRCS:.c=.o)
PROG = logger

C_FLAGS = -Werror -g 
LD_FLAGS = -lpthread

all: $(PROG)

$(PROG): $(SRCS) $(HDRS)
	$(CC) -c $(C_FLAGS) $(SRCS)
	$(CC) -o $(PROG) $(OBJS) $(LD_FLAGS) 

clean:
	$(RM) $(PROG) $(OBJS)
