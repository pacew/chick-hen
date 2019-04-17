#! /usr/bin/env python3

import sys
import socket
import json
import hashlib
import re
import pprint
import unittest
import hmac
import time
import sqlite3

BROADCAST_NODENUM = 99
MY_NODENUM = 98

CHICK_HEN_MADDR = "224.0.0.130"
CHICK_HEN_PORT = 32519

pp = pprint.PrettyPrinter(indent=4)

def dump(buf):
   if buf is None:
      print("None")
      return
   
   start = 0
   while start < len(buf):
      print('{0:04x}:'.format(start), end='')
      for off in range (0, 16):
         if (start + off >= len(buf)):
            break
         print(' {0:02x}'.format(buf[start+off]), end='')
      print('')
      start += 16


def load_protocol(filename):
   global the_protocol
   with open(filename) as file:
      the_protocol = json.load(file)
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
      encode_val (pb, val, bits)

def decode(pb, pkt_name):
   global the_protocol

   desc = the_protocol['pkts'][pkt_name]
   pval = {}
   for fdata in desc['fields']:
      field_name = fdata['field']
      bits = fdata['bits']
      val = decode_val (pb, bits)
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
         if rpkt[0] == MY_NODENUM:
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
      if rpkt != None:
         return rpkt

   return None

def init():
   global sock
   load_protocol("proto-gen.json")
   sock = socket.socket (socket.AF_INET, socket.SOCK_DGRAM)

conn = None
cursor = None

def query(stmt, args=()):
   global conn
   global cursor
   if conn == None:
      conn = sqlite3.connect("hen.db")
      conn.row_factory = sqlite3.Row
      cursor = conn.cursor()
   return cursor.execute(stmt, args)

def fetch(cur=None):
   global cursor
   if cur == None:
      cur = cursor
   return cur.fetchone()

def commit():
   global conn
   conn.commit()

def get_seq():
   query("select lastval"+
         " from seq")
   r = fetch()
   if r == None:
      val = 100
      query("insert into seq(lastval) values (?)", (val,))
   else:
      val = r['lastval'] + 1
      query("update seq set lastval = ?", (val,))
   commit()
   return val

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
