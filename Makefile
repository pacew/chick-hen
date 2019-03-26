CFLAGS = -g -Wall
LIBS = -lm

all: hen chick

hen: hen.o
	$(CC) $(CFLAGS) -o hen hen.o

chick: chick.o
	$(CC) $(CFLAGS) -o chick chick.o $(LIBS)
