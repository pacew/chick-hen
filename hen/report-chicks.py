#! /usr/bin/env python3

import sys
import requests
import json
import hmac
import base64
import urllib
import re

sys.path.insert(0, "/home/pace/psite")  # noqa: E402
import psite

hen_name = psite.getvar("hen_name")
hen_key_hex = psite.getvar("hen_key")
coop_url = psite.getvar("coop_url")

hen_key = bytes.fromhex(hen_key_hex)


def apicall(params):
    global hen_key

    payload = params.copy()
    payload['hen_name'] = hen_name
    payload_bytes = bytes(json.dumps(payload), "utf8")

    sig_binary = hmac.new(hen_key, payload_bytes, "sha256").digest()
    sig_base64 = base64.b64encode(sig_binary)
    args = dict(payload=payload_bytes, sig=sig_base64)

    endpoint = re.sub(r"/*$", "", coop_url) + "/api.php"

    full_url = "{}?{}&debug=1".format(endpoint, urllib.parse.urlencode(args))
    print(full_url)

    r = requests.post(endpoint, args)

    try:
        val = r.json()
    except(json.decoder.JSONDecodeError):
        err_text = psite.strip_tags(r.text)
        val = dict(err=err_text)

    return (val)


macs = []
psite.query("select chick_mac from chicks")
while True:
    row = psite.fetch()
    if row is None:
        break
    chick_mac = row[0]
    macs.append(chick_mac)

args = {}
args['op'] = "report_chicks"
args['macs'] = macs

val = apicall(args)
if 'err' in val:
    print("error:", val['err'])
    sys.exit(0)

print(val)
