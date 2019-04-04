CFLAGS = -g -Wall
LIBS = -lm

all: hen chick

HEN_OBJS = hen.o wire.o multicast.o
hen: $(HEN_OBJS)
	$(CC) $(CFLAGS) -o hen $(HEN_OBJS)

CHICK_OBJS = chick.o wire.o multicast.o
chick: $(CHICK_OBJS)
	$(CC) $(CFLAGS) -o chick $(CHICK_OBJS) $(LIBS)

clean:
	rm -f hen chick
	rm -f *.o *~
