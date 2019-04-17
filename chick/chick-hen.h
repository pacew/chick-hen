#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <math.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "multicast.h"
#include "proto-gen.h"

#define CHICK_HEN_MADDR "224.0.0.130"
#define CHICK_HEN_PORT 32519



