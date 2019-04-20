<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_attach = intval (@$_REQUEST['attach']);
$arg_chick_mac = trim (@$_REQUEST['chick_mac']);
$arg_hen_id = intval (@$_REQUEST['hen_id']);

if ($arg_attach == 1) {
    $q = query ("select hen_id"
                ." from chicks"
                ." where chick_mac = ?",
                $arg_chick_mac);
    if (($r = fetch ($q)) == NULL) {
        query ("insert into chicks (chick_mac, hen_id) values (?,?)",
               array ($arg_chick_mac, $arg_hen_id));
    } else {
        query ("update chicks set hen_id = ? where chick_mac = ?",
               array ($arg_hen_id, $arg_chick_mac));
    }

    redirect ("setup.php");
}

$q = query ("select chick_name, hen_id"
            ." from chicks"
            ." where chick_mac = ?",
            $arg_chick_mac);
if (($r = fetch ($q)) == NULL) {
    $body .= "not found";
    pfinish ();
}

$db_chick_name = trim ($r->chick_name);
$db_hen_id = intval ($r->hen_id);

$q = query ("select hen_name"
            ." from hens"
            ." where hen_id = ?",
            $db_hen_id);
if (($r = fetch ($q)) == NULL) {
    $db_hen_name = "";
} else {
    $db_hen_name = trim ($r->hen_name);
}

$body .= "<table class='twocol'>\n";
$body .= "<tr><th>chick_mac</th><td>";
$body .= h($arg_chick_mac);
$body .= "</td></tr>\n";
$body .= "<tr><th>chick_name</th><td>";
$body .= h($db_chick_name);
$body .= "</td></tr>\n";
$body .= "<tr><th>hen_name</th><td>";

$t = sprintf ("hen.php?hen_id=%d", $db_hen_id);
$body .= mklink($db_hen_name, $t);
$body .= "</td></tr>\n";
$body .= "</table>\n";


pfinish();
    
