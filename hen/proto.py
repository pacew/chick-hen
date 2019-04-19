#! /usr/bin/env python3

import socket
import json
import unittest
import hmac

CHICK_HEN_MADDR = "224.0.0.130"
CHICK_HEN_PORT = 32519


def dump(buf):
    if buf is None:
        print("None")
        return

    start = 0
    while start < len(buf):
        print('{0:04x}:'.format(start), end='')
        for off in range(0, 16):
            if (start + off >= len(buf)):
                break
            print(' {0:02x}'.format(buf[start+off]), end='')
            print('')
        start += 16


def load_protocol(filename):
    global the_protocol
    with open(filename) as file:
        the_protocol = json.load(file)
    global BROADCAST_MAC_HASH, HEN_MAC_HASH
    BROADCAST_MAC_HASH = the_protocol['BROADCAST_MAC_HASH']
    HEN_MAC_HASH = the_protocol['HEN_MAC_HASH']
    global system_key
    with open(".system_key") as file:
        system_key = bytes.fromhex(file.readline().strip())


def get_op(name):
    global the_protocol
    return the_protocol['pkts'][name]['op']


def encode_init():
    return dict(buf=bytearray(), avail_bits=0)


def encode_val(pb, val, bits):
    togo = bits
    remaining_val = val
    buf = pb['buf']
    avail_bits = pb['avail_bits']
    while togo > 0:
        if avail_bits == 0:
            buf.append(0)
            avail_bits = 8
        thistime = avail_bits
        if thistime > togo:
            thistime = togo
            mask = (1 << thistime) - 1
            buf[-1] |= (remaining_val & mask) << (8 - avail_bits)

        avail_bits -= thistime
        togo -= thistime
        remaining_val >>= thistime
    pb['avail_bits'] = avail_bits


def sign(pb):
    d = compute_digest(pb['buf'])
    pb['buf'].extend(d)


def decode_init(full_buf):
    global system_key
    buf = full_buf[0:-4]
    sig = full_buf[-4:]

    d = compute_digest(buf)
    sig_ok = (sig == d)

    return dict(buf=buf, avail_bits=len(buf)*8, used_bits=0, sig_ok=sig_ok)


def decode_val(pb, bits):
    togo = bits
    output = 0
    output_shift = 0
    buf = pb['buf']
    used_bits = pb['used_bits']
    avail_bits = pb['avail_bits']
    while togo > 0:
        byte_off = used_bits // 8
        bit_off = used_bits & 7
        thistime = 8 - bit_off
        if thistime > togo:
            thistime = togo
        if used_bits + thistime > avail_bits:
            break

        mask = (1 << thistime) - 1
        nibble = (buf[byte_off] >> bit_off) & mask
        output |= nibble << output_shift

        togo -= thistime
        used_bits += thistime
        output_shift += thistime
    pb['used_bits'] = used_bits
    return output


def encode(pb, pkt_name, pval):
    global the_protocol
    desc = the_protocol['pkts'][pkt_name]
    for fdata in desc['fields']:
        field_name = fdata['field']
        bits = fdata['bits']
        val = pval[field_name]
        encode_val(pb, val, bits)


def decode(pb, pkt_name):
    global the_protocol

    desc = the_protocol['pkts'][pkt_name]
    pval = {}
    for fdata in desc['fields']:
        field_name = fdata['field']
        bits = fdata['bits']
        val = decode_val(pb, bits)
        pval[field_name] = val
    return pval


def compute_digest(buf):
    global system_key
    h = hmac.new(system_key, buf, "sha256")
    d = h.digest()
    return (d[0:4])


def send(msg):
    global sock
    sock.sendto(msg, (CHICK_HEN_MADDR, CHICK_HEN_PORT))


def rcv(delay=0):
    global sock

    if delay == 0:
        delay = 0.250

    while True:
        try:
            sock.settimeout(delay)
            (rpkt, addr) = sock.recvfrom(10000)
            if rpkt[0] == BROADCAST_MAC_HASH or rpkt[0] == HEN_MAC_HASH:
                return rpkt
        except socket.timeout:
            break

    return None


def send_rcv(msg, retries=0, delay=0):
    if retries == 0:
        retries = 2

    for retry in range(0, retries):
        send(msg)
        rpkt = rcv(delay)
        if rpkt is not None:
            return rpkt

    return None


def init():
    global sock
    load_protocol("proto-gen.json")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


class TestProto(unittest.TestCase):
    def test_proto(self):
        load_protocol("proto-gen.json")

        cfg = dict(idx=1, nbits=2, input_chan=3, options=4, last=5)
        pb = encode_init()

        encode(pb, 'chan_config', cfg)
        sign(pb)

        buf = pb['buf']
        pb = decode_init(buf)
        pval = decode(pb, 'chan_config')
        print(pval)
        print(pb)
