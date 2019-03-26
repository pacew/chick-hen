CFLAGS = -g -Wall

all: hen

hen: hen.o
	$(CC) $(CFLAGS) -o hen hen.o
