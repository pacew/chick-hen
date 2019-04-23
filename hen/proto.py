#! /usr/bin/env python3

import socket
import json
import unittest
import hmac
import hashlib

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
    global BROADCAST_MAC_HASH, HEN_MAC_HASH, SUBSTITUTE_MAC_HASH
    BROADCAST_MAC_HASH = the_protocol['BROADCAST_MAC_HASH']
    HEN_MAC_HASH = the_protocol['HEN_MAC_HASH']
    SUBSTITUTE_MAC_HASH = the_protocol['SUBSTITUTE_MAC_HASH']


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
        if thistime >= togo:
            thistime = togo
        mask = (1 << thistime) - 1
        buf[-1] |= (remaining_val & mask) << (8 - avail_bits)

        avail_bits -= thistime
        togo -= thistime
        remaining_val >>= thistime
    pb['avail_bits'] = avail_bits


def sign_with_hen_key(pb):
    d = compute_hen_digest(pb['buf'])
    pb['buf'].extend(d)


def sign_with_chick_key(pb, chick_mac_bin):
    d = compute_chick_digest(pb['buf'], chick_mac_bin)
    pb['buf'].extend(d)


def decode_init(full_buf):
    buf = full_buf[0:-4]
    sig = full_buf[-4:]

    d = compute_hen_digest(buf)
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


def compute_hen_digest(buf):
    global hen_key_bin
    d = hmac.new(hen_key_bin, buf, "sha256").digest()
    return d[0:4]


def compute_chick_digest(buf, chick_mac_bin):
    global hen_key_bin
    key = hen_key_bin + chick_mac_bin
    d = hmac.new(key, buf, "sha256").digest()
    return d[0:4]


# first 31 bits of sha256 of buffer as a non-neg integer (little endian)
def simple_digest(buf):
    full_digest = hashlib.sha256(buf).digest()
    b0 = full_digest[0]
    b1 = full_digest[1]
    b2 = full_digest[2]
    b3 = full_digest[3]
    return b0 | (b1 << 8) | (b2 << 16) | ((b3 & 0x7f) << 24)


def mac_to_bin(mac):
    octet_strings = mac.split(":")
    octet_bytes = map(lambda x: int(x, 16), octet_strings)
    mac_bin = bytes(octet_bytes)
    return mac_bin


def compute_mac_hash(mac_bin):
    mac_hash = simple_digest(mac_bin) & 0xff
    if mac_hash in [BROADCAST_MAC_HASH, HEN_MAC_HASH, 0, 0xff]:
        mac_hash = SUBSTITUTE_MAC_HASH
    return mac_hash


def send(msg):
    global sock
    sock.sendto(msg, (CHICK_HEN_MADDR, CHICK_HEN_PORT))


def rcv(delay=0.250):
    global sock

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


def init(key):
    load_protocol("proto-gen.json")

    global sock
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, 1)

    mreq = socket.inet_aton(CHICK_HEN_MADDR) + socket.inet_aton("0.0.0.0")
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    # ? prevents receiving responses
    # sock.bind((CHICK_HEN_MADDR, CHICK_HEN_PORT))

    global hen_key_bin
    hen_key_bin = bytes.fromhex(key)


class TestProto(unittest.TestCase):
    def test_proto(self):
        load_protocol("proto-gen.json")

        cfg = dict(idx=1, nbits=2, input_chan=3, options=4, last=5)
        pb = encode_init()

        encode(pb, 'chan_config', cfg)
        sign_with_hen_key(pb)

        buf = pb['buf']
        pb = decode_init(buf)
        pval = decode(pb, 'chan_config')
        print(pval)
        print(pb)
