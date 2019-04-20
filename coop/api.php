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

$ret = (object)NULL;
$ret->foo = "bar";
$ret->params = $_REQUEST;

json_finish ($ret);
