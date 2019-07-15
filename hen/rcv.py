#! /usr/bin/env python3
import sys
import io
import csv
import datetime

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto
import chicklib

vflag = False

hen_key = psite.getvar("hen_key")

proto.init(hen_key)

chicks = chicklib.get_chicks()
print(chicks)

chicks_by_mac_hash = {}

for chick in chicks:
    mac_hash = chick['mac_hash']
    if mac_hash not in chicks_by_mac_hash:
        chicks_by_mac_hash[mac_hash] = []
    chicks_by_mac_hash[mac_hash].append(chick)


def process_dpoint(chick, pb):
    print("process_dpoint", chick, pb)
    proto.roundup(pb)
    dpoint = proto.decode(pb, 'dpoint')
    psite.query("select 0"
                " from dpoints"
                " where chick_mac = ?"
                "   and ts_raw = ?",
                (chick['mac'], dpoint['timestamp']))
    if psite.fetch() is None:
        vals = []
        for chan in chick['chanlist']:
            vals.append(proto.decode_val(pb, chan['bit_width']))
            f = io.StringIO()
            c = csv.writer(f)
            c.writerow(vals)
        val_string = f.getvalue().strip()
        seq = psite.get_seq()
        dttm = datetime.datetime.fromtimestamp(dpoint['timestamp'])
        psite.query("insert into dpoints(seq, chick_mac, chanlist_id,"
                    "  ts_raw, dttm, vals"
                    ") values (?,?,?,?,?,?)",
                    (seq, chick['mac'], chick['chanlist_id'],
                     dpoint['timestamp'], dttm, val_string))
        psite.commit()

    hdr = {}
    hdr['mac_hash'] = chick['mac_hash']
    hdr['op'] = proto.get_op('ack_dpoint')
    pb = proto.encode_init()
    proto.encode(pb, "hdr", hdr)

    ack ={}
    ack['timestamp'] = dpoint['timestamp']
    proto.encode(pb, "ack_dpoint", ack)

    proto.sign_with_chick_key(pb, chick['mac_bin'])

    proto.send(pb['buf'])

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
                    rsig = proto.compute_chick_digest(payload,
                                                      chick['mac_bin'])
                    if rsig == sig:
                        print("match")
                        process_dpoint(chick, pb)
                        break


try:
    rcv()
except(KeyboardInterrupt):
    sys.exit(0)
