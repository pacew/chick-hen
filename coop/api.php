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
        $q = query ("select 0"
                    ." from reported_chicks"
                    ." where hen_name = ?"
                    ."   and chick_mac = ?",
                    array ($hen_name, $mac));
        if (fetch ($q) == NULL) {
            query ("insert into reported_chicks (hen_name, chick_mac)"
                   ." values (?,?)",
                   array ($hen_name, $mac));
        }
    }

    $ret = (object)NULL;
    $ret->status = "ok";
    json_finish ($ret);
}
    
