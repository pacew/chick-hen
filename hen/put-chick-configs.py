#! /usr/bin/env python3

import sys
import random

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto

hen_name = psite.getvar("hen_name")
hen_key_hex = psite.getvar("hen_key")
hen_key = bytes.fromhex(hen_key_hex)


def make_chick_config(chick_mac, xmit_cookie):
    print("config", chick_mac)

    psite.query("select chanlist_id from chicks where chick_mac = ?",
                (chick_mac,))
    r = psite.fetch()
    if r is None:
        print("no info for", chick_mac)
        return
    chanlist_id = int(r[0])
    psite.query("select chanlist_name from chanlists where chanlist_id = ?",
                (chanlist_id,))
    r = psite.fetch()
    if r is None:
        print("no chanlist info for", chanlist_id)
        return
    chanlist_name = r[0]
    print("chick", chick_mac, "chanlist", chanlist_id, chanlist_name)

    chick_mac_bin = proto.mac_to_bin(chick_mac)
    chick_mac_hash = proto.compute_mac_hash(chick_mac_bin)

    hdr = {}
    hdr['mac_hash'] = chick_mac_hash
    hdr['op'] = proto.get_op('chanlist')

    pb = proto.encode_init()
    proto.encode(pb, "hdr", hdr)

    cookie_pkt = {}
    cookie_pkt['cookie'] = xmit_cookie
    proto.encode(pb, "cookie", cookie_pkt)

    q = psite.query("select chan_type, port, bit_width, bit_position"
                    " from chans"
                    " where chanlist_id = ?"
                    " order by sort_order, chanlist_id",
                    (chanlist_id,))
    for row in q:
        elt = {}
        elt['chan_type'] = int(row[0])
        elt['port'] = int(row[1])
        elt['bit_width'] = int(row[2])
        elt['bit_position'] = int(row[3])
        proto.encode(pb, "chanlist", elt)

    proto.sign_with_chick_key(pb, chick_mac_bin)

    return (pb['buf'])


def sendrcv(pkt, cookie):
    for retries in range(0, 2):
        proto.send(pkt)
        while True:
            rpkt = proto.rcv()
            if rpkt is None:
                break
            proto.dump(rpkt)
            pb = proto.decode_init(rpkt)
            hdr = proto.decode(pb, 'hdr')
            if hdr['mac_hash'] != proto.HEN_MAC_HASH:
                continue
            cookie_pkt = proto.decode(pb, 'cookie')
            if int(cookie_pkt['cookie']) != cookie:
                continue
            return pb
    return None


def put_chick_config(chick_mac):
    cookie = random.randint(0, 0x7fffffff)

    pkt = make_chick_config(chick_mac, cookie)
    print("xmit for", chick_mac, "cookie", cookie)
    proto.dump(pkt)

    pb = sendrcv(pkt, cookie)
    if pb is None:
        print("no response")
        return
    ack = proto.decode(pb, 'ack')
    if ack['err'] != 0:
        print("chick error", ack)
        return
    print("success")
    return True


hen_key = psite.getvar("hen_key")
proto.init(hen_key)

if len(sys.argv) == 1:
    q = psite.query("select chick_mac from chicks order by chick_mac")
    for row in q:
        chick_mac = row[0]
        put_chick_config(chick_mac)
else:
    for chick_mac in sys.argv[1:]:
        put_chick_config(chick_mac)
