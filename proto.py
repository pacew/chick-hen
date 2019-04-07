#! /usr/bin/env python3

import sys
import socket
import json
import hashlib
import re
import pprint
import unittest

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
   try:
      with open (filename) as json_file:
         the_protocol = json.load (json_file)
   except (OSError, ValueError):
      raise ValueError

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

def decode_init(buf):
   return dict(buf=buf, avail_bits=len(buf)*8, used_bits=0)

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
   pval = {}
   desc = the_protocol['pkts'][pkt_name]
   for fdata in desc['fields']:
      field_name = fdata['field']
      bits = fdata['bits']
      val = decode_val (pb, bits)
      pval[field_name] = val
   return pval
      

class TestProto(unittest.TestCase):
   def test_proto(self):
      load_protocol("proto-gen.json")
      
      cfg = dict(idx=1, nbits=2, input_chan=3, options=4, last=5)
      pb = encode_init ()

      encode (pb, 'chan_config', cfg)
      dump (pb['buf'])

      buf = pb['buf']
      pb = decode_init (buf)
      pval = decode (pb, 'chan_config')
      print (pval)
