#! /usr/bin/env python3
import sys

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto

vflag = False

hen_key = psite.getvar("hen_key")

proto.init(hen_key)

chicks_by_mac_hash = {}

psite.query("select chick_mac"
            " from chicks")
while True:
    r = psite.fetch()
    if r is None:
        break;

    chick_mac = r[0]

    chick_mac_bin = proto.mac_to_bin(chick_mac)
    chick_mac_hash = proto.compute_mac_hash(chick_mac_bin)

    chick = dict(mac=chick_mac, mac_bin=chick_mac_bin)

    if chick_mac_hash not in chicks_by_mac_hash:
        chicks_by_mac_hash[chick_mac_hash] = []

    chicks_by_mac_hash[chick_mac_hash].append(chick)

def rcv():
    dpoint_op = proto.get_op('dpoint')

    while True:
        rpkt = proto.rcv()
        if rpkt is not None:
            proto.dump(rpkt)
            payload = rpkt[0:-4]
            sig = rpkt[-4:]

            pb = proto.decode_init(rpkt)
            hdr = proto.decode(pb, 'hdr')
            if hdr['mac_hash'] != proto.HEN_MAC_HASH:
                continue
            if hdr['op'] != dpoint_op:
                continue
            from_mac_hash = proto.decode(pb, 'from_mac_hash')
            chicks = chicks_by_mac_hash.get(from_mac_hash['mac_hash'])
            if chicks is not None:
                for chick in chicks:
                    print(chick)
                    rsig = proto.compute_chick_digest(payload, chick['mac_bin'])
                    if rsig == sig:
                        print("match")
                        break

try:
    rcv()
except(KeyboardInterrupt):
    sys.exit(0)
