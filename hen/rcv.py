#! /usr/bin/env python3
import sys

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto

vflag = False

hen_key = psite.getvar("hen_key")

print(hen_key)
proto.init(hen_key)

def rcv():
    while True:
        rpkt = proto.rcv()
        proto.dump(rpkt)

try:
    rcv()
except(KeyboardInterrupt):
    sys.exit(0)
