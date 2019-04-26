#! /usr/bin/env python3
import sys

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto

vflag = False

hen_key = psite.getvar("hen_key")

proto.init(hen_key)


def scan_response(chick_mac):
    psite.query("select 0"
                " from chicks"
                " where chick_mac = ?",
                (chick_mac,))
    r = psite.fetch()
    if r is None:
        print("new chick", chick_mac)

        psite.query("insert into chicks(chick_mac) values(?)",
                    (chick_mac,))
    else:
        print("dup chick", chick_mac)

    psite.query("update chicks set last_ts = datetime('now')"
                "where chick_mac = ?",
                (chick_mac,))
    psite.commit()


def scan(key, divisor):
    for rem in range(0, divisor):
        if vflag:
            print("scan", rem, "of", divisor)
        scan = dict(key=key, divisor=divisor, remainder=rem, jitter=0)

        hdr = {}
        hdr['mac_hash'] = proto.BROADCAST_MAC_HASH
        hdr['op'] = proto.get_op('scan')

        pb = proto.encode_init()
        proto.encode(pb, "hdr", hdr)
        proto.encode(pb, "scan", scan)
        proto.sign_with_hen_key(pb)
        if vflag:
            proto.dump(pb['buf'])

        proto.send(pb['buf'])
        probe_response_op = proto.get_op('probe_response')
        while True:
            rpkt = proto.rcv()
            if rpkt is None:
                break
            if vflag:
                proto.dump(rpkt)
            pb = proto.decode_init(rpkt)
            hdr = proto.decode(pb, 'hdr')
            if hdr['op'] == probe_response_op:
                resp = proto.decode(pb, 'probe_response')
                mac = '{0:02x}:{1:02x}:{2:02x}:{3:02x}:{4:02x}:{5:02x}'.format(
                    resp['mac0'], resp['mac1'], resp['mac2'],
                    resp['mac3'], resp['mac4'], resp['mac5'])
                scan_response(mac)


primes = [2, 3, 5, 7, 11, 13, 17]

try:
    for scan_num in range(0, len(primes)):
        prime = primes[scan_num]
        print("scan {0}".format(prime))
        scan(scan_num, prime)
except(KeyboardInterrupt):
    sys.exit(0)
