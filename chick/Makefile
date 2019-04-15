CFLAGS = -g -Wall
LIBS = -lcrypto -lm

all: chick

CHICK_OBJS = chick.o wire.o multicast.o proto-common.o proto-gen.o digest.o
chick: $(CHICK_OBJS)
	$(CC) $(CFLAGS) -o chick $(CHICK_OBJS) $(LIBS)

$(CHICK_OBJS) $(HEN_OBJS): proto-common.h proto-gen.h

proto-gen.h proto-gen.c proto-gen.json: ../hen/genproto ../hen/chick-hen.proto
	../hen/genproto ../hen/chick-hen.proto

clean:
	rm -f hen chick
	rm -f *.o *~
	rm -f proto-gen.[ch] proto-gen.json

test:
	python3 -m unittest proto.py
