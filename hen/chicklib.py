import sys

sys.path.insert(0, "../../psite")  # noqa: E402
import psite

import proto


def get_chanlist(chick):
    psite.query("select chanlist_id from chicks where chick_mac = ?",
                (chick['mac'],))
    r = psite.fetch()
    chanlist_id = int(r[0]) if r is not None else 0

    psite.query("select chanlist_name from chanlists where chanlist_id = ?",
                (chanlist_id,))
    r = psite.fetch()
    chanlist_name = r[0] if r is not None else ''

    q = psite.query("select hwchan_name, chan_type, port,"
                    "  bit_width, bit_position"
                    " from chans"
                    " where chanlist_id = ?"
                    " order by sort_order",
                    (chanlist_id,))
    chanlist = []
    for row in q:
        elt = {}
        elt['hwchan_name'] = row[0]
        elt['chan_type'] = int(row[1])
        elt['port'] = int(row[2])
        elt['bit_width'] = int(row[3])
        elt['bit_position'] = int(row[4])
        chanlist.append(elt)

    chick['chanlist_id'] = chanlist_id
    chick['chanlist_name'] = chanlist_name
    chick['chanlist'] = chanlist


def get_chicks():
    chicks = []

    psite.query("select chick_mac"
                " from chicks")
    while True:
        r = psite.fetch()
        if r is None:
            break
        mac = r[0]

        mac_bin = proto.mac_to_bin(mac)
        mac_hash = proto.compute_mac_hash(mac_bin)

        chick = dict(mac=mac, mac_bin=mac_bin, mac_hash=mac_hash)
        chicks.append(chick)

    for chick in chicks:
        get_chanlist(chick)

    return chicks
