CFLAGS = -g -Wall
LIBS = -lm

all: hen chick

hen: hen.o wire.o
	$(CC) $(CFLAGS) -o hen hen.o wire.o

chick: chick.o wire.o
	$(CC) $(CFLAGS) -o chick chick.o wire.o $(LIBS)

clean:
	rm -f hen chick
	rm -f *.o *~
