#! /usr/bin/env python3

import sys
import requests
import json
import hmac
import base64
import urllib

with open(".hen_key") as f:
    hen_key = bytes.fromhex(f.readline().strip())


def apicall(params):
    global hen_key

    payload = bytes(json.dumps(params), "utf8")

    sig_binary = hmac.new(hen_key, payload, "sha256").digest()
    sig_base64 = base64.b64encode(sig_binary)
    args = dict(payload=payload, sig=sig_base64)

    endpoint = "https://k.pacew.org:12912/api.php"

    full_url = "{}?{}".format(endpoint, urllib.parse.urlencode(args))
    print(full_url)

    r = requests.post(endpoint, args)
    return r.json()


try:
    val = apicall(dict(arg1='123', arg2='abc'))
except(json.decoder.JSONDecodeError):
    print("can't parse apicall result")
    sys.exit(1)

print(val)
