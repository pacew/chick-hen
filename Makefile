CFLAGS = -g -Wall
LIBS = -lm

all: hen chick

HEN_OBJS = hen.o wire.o multicast.o proto-common.o proto-gen.o
hen: $(HEN_OBJS)
	$(CC) $(CFLAGS) -o hen $(HEN_OBJS)

CHICK_OBJS = chick.o wire.o multicast.o proto-common.o proto-gen.o
chick: $(CHICK_OBJS)
	$(CC) $(CFLAGS) -o chick $(CHICK_OBJS) $(LIBS)

$(CHICK_OBJS) $(HEN_OBJS): proto-common.h proto-gen.h

proto-gen.h proto-gen.c proto-gen.json: genproto chick-hen.proto
	./genproto

clean:
	rm -f hen chick
	rm -f *.o *~
	rm proto-gen.[ch] proto-gen.json

test:
	python3 -m unittest proto.py
