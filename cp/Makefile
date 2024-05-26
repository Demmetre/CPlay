CC = gcc
CFLAGS = 
LDFLAGS = 

BINDIR = bin

SRCS = main.c util.c
OBJS = $(SRCS:.c=.o)
EXEC = my_program

.PHONY: all clean

all: $(EXEC) move

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(EXEC)

move:
	mkdir -p $(BINDIR)
	mv $(OBJS) $(BINDIR)
