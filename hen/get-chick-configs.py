#! /usr/bin/env python3

import sys
import requests
import json
import hmac
import base64
import urllib

sys.path.insert(0, "/home/pace/psite")  # noqa: E402
import psite

hen_name = psite.getvar("hen_name")
hen_key_hex = psite.getvar("hen_key")

hen_key = bytes.fromhex(hen_key_hex)


def apicall(params):
    global hen_key

    payload = params.copy()
    payload['hen_name'] = hen_name
    payload_bytes = bytes(json.dumps(payload), "utf8")

    sig_binary = hmac.new(hen_key, payload_bytes, "sha256").digest()
    sig_base64 = base64.b64encode(sig_binary)
    args = dict(payload=payload_bytes, sig=sig_base64)

    endpoint = "https://k.pacew.org:12912/api.php"

    full_url = "{}?{}&debug=1".format(endpoint, urllib.parse.urlencode(args))
    print(full_url)

    r = requests.post(endpoint, args)

    try:
        val = r.json()
    except(json.decoder.JSONDecodeError):
        err_text = psite.strip_tags(r.text)
        val = dict(err=err_text)

    return (val)


args = {}
args['op'] = "get_chick_configs"

val = apicall(args)
if 'err' in val:
    print("error:", val['err'])
    sys.exit(0)

for chick in val['chicks']:
    chick_mac = chick['chick_mac']
    chanlist_id = int(chick['chanlist_id'])
    psite.query("update chicks set chanlist_id = ? where chick_mac = ?",
                (chanlist_id, chick_mac))

psite.query("delete from chanlists")
psite.query("delete from chans")

for idx in val['chanlists']:
    chanlist = val['chanlists'][idx]
    chanlist_id = int(chanlist['chanlist_id'])
    chanlist_name = chanlist['chanlist_name']

    psite.query("insert into chanlists(chanlist_id, chanlist_name)"
                "values (?,?)",
                (chanlist_id, chanlist_name))

    sort_order = 1
    for chan in chanlist['chans']:
        hwchan_name = chan['hwchan_name']
        chan_type = int(chan['chan_type'])
        port = int(chan['port'])
        bit_width = int(chan['bit_width'])
        bit_position = int(chan['bit_position'])

        psite.query("insert into chans (chanlist_id, sort_order, "
                    "  hwchan_name, chan_type, port, bit_width, bit_position"
                    ") values (?,?,?,?,?,?,?)",
                    (chanlist_id, sort_order, hwchan_name, chan_type, port,
                     bit_width, bit_position))
        sort_order = sort_order + 1

psite.commit()
