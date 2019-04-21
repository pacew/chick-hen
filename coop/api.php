<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

$arg_payload = trim (@$_REQUEST['payload']);
$arg_sig_base64 = trim (@$_REQUEST['sig']);

$arg_sig_hex = bin2hex (base64_decode ($arg_sig_base64));

$q = query ("select key"
            ." from hens");
$r = fetch ($q);
$hen_key = hex2bin($r->key);

$computed_sig_hex = hash_hmac ("sha256", $arg_payload, $hen_key);

if (strcmp ($computed_sig_hex, $arg_sig_hex) != 0) {
    $ret = (object)NULL;
    $ret->err = "bad signature";
    json_finish ($ret);
}

$args = json_decode ($arg_payload, true);
$op = @$args['op'];

switch ($op) {
case "report_chicks":
    do_report_chicks ($args);
    break;
case "get_chick_configs":
    do_get_chick_configs ($args);
    break;
default:
    $ret = (object)NULL;
    $ret->err = "unknown op";
    $ret->op = $op;
    json_finish ($ret);
}

$ret = (object)NULL;
$ret->err = "unhandled op";
json_finish ($ret);


function do_report_chicks ($args) {
    $hen_name = $args['hen_name'];
    
    foreach ($args['macs'] as $mac) {
        $q = query ("select hen_name"
                    ." from reported_chicks"
                    ." where chick_mac = ?",
                    $mac);
        if (($r = fetch ($q)) == NULL) {
            query ("insert into reported_chicks (hen_name, chick_mac)"
                   ." values (?,?)",
                   array ($hen_name, $mac));
        } else if (strcmp ($r->hen_name, $hen_name) != 0) {
            query ("update reported_chicks set hen_name = ? where chick_mac = ?",
                   array ($hen_name, $mac));
        }
    }

    $ret = (object)NULL;
    $ret->status = "ok";
    json_finish ($ret);
}
    
function do_get_chick_configs ($args) {
    $hen_name = $args['hen_name'];
    
    $q = query ("select hen_id"
                ." from hens"
                ." where hen_name = ?",
                $hen_name);
    if (($r = fetch ($q)) == NULL) {
        $ret = (object)NULL;
        $ret->err = "hen not found";
        json_finish ($ret);
    }
    $hen_id = intval ($r->hen_id);

    $q = query ("select chick_mac, chanlist_id"
                ." from chicks"
                ." where hen_id = ?",
                $hen_id);
    $chicks = array();
    while (($r = fetch ($q)) != NULL) {
        $cp = (object)NULL;
        $cp->chick_mac = trim ($r->chick_mac);
        $cp->chanlist_id = intval ($r->chanlist_id);
        $chicks[] = $cp;
        

    }
        
    $q = query ("select c.chanlist_id, c.chanlist_name"
                ." from chanlists c"
                ." where c.chanlist_id in (select distinct ch.chanlist_id"
                ."                          from chicks ch"
                ."                          where ch.hen_id = ?)",
                $hen_id);
    $chanlists = array ();
    while (($r = fetch ($q)) != NULL) {
        $chanlist_id = intval ($r->chanlist_id);
        
        $cp = (object)NULL;
        $cp->chanlist_id = $chanlist_id;
        $cp->chanlist_name = trim ($r->chanlist_name);
        $cp->chans = array ();

        $chanlists[$chanlist_id] = $cp;
    }

    $q = query ("select h.chanlist_id, h.hwchan_name, h.chan_type, h.port,"
                ."   h.bit_width, h.bit_position"
                ." from hwchan h"
                ." where h.chanlist_id in"
                ."   (select distinct c.chanlist_id"
                ."      from chicks c"
                ."      where c.hen_id = ?)"
                ." order by h.chanlist_id, h.sort_order, h.hwchan_id",
                $hen_id);
    while (($r = fetch ($q)) != NULL) {
        $chanlist_id = intval ($r->chanlist_id);
        
        $hp = (object)NULL;
        $hp->hwchan_name = trim ($r->hwchan_name);
        $hp->chan_type = intval ($r->chan_type);
        $hp->port = intval ($r->port);
        $hp->bit_width = intval ($r->bit_width);
        $hp->bit_position = intval ($r->bit_position);

        if (($cp = @$chanlists[$chanlist_id]) != NULL) {
            $cp->chans[] = $hp;
        }
    }
        

    $ret = (object)NULL;
    $ret->chicks = $chicks;
    $ret->chanlists = $chanlists;
    json_finish ($ret);

       
}
