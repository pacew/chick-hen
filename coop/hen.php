<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$body .= "<h1>hen</h1>\n";

$arg_edit = intval(@$_REQUEST['edit']);
$arg_delete = intval(@$_REQUEST['delete']);
$coop_id = intval(@$_REQUEST['coop_id']);
$arg_hen_id = intval(@$_REQUEST['hen_id']);
$arg_hen_name = trim(@$_REQUEST['hen_name']);

$db_hen_name = "";
$db_key_text = "";
$db_key = "";

if ($arg_hen_id) {
    $q = query ("select coop_id, hen_name, key_text, key"
        ." from hens"
        ." where hen_id = ?",
        $arg_hen_id);
    if (($r = fetch ($q)) != NULL) {
        $coop_id = intval ($r->coop_id);
        $db_hen_name = trim ($r->hen_name);
        $db_key_text = trim ($r->key_text);
        $db_key = trim ($r->key);
    }
}

if ($coop_id) {
    $body .= "<div>\n";
    $t = sprintf ("coop.php?coop_id=%d", $coop_id);
    $body .= mklink ("back to coop", $t);
    $body .= "</div>\n";
}

if ($arg_edit == 1) {
    $body .= "<form action='hen.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='coop_id' value='%d' />\n",
        $coop_id);
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
        $arg_hen_id);
    
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>Name</th><td>\n";
    $body .= sprintf ("<input type='text' name='hen_name' value='%s' />\n",
        $db_hen_name);
    $body .= "</td></tr>\n";

    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Save' />\n";
    if ($coop_id) {
        $t = sprintf ("coop.php?coop_id=%d", $coop_id);
    } else {
        $t = "coops.php";
    }
    $body .= mklink ("cancel", $t);
    if ($arg_hen_id) {
        $body .= " | ";
        $t = sprintf ("hen.php?hen_id=%d&delete=1", $arg_hen_id);
        $body .= mklink ("delete", $t);
    }
            
    $body .= "</td></tr>\n";
    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_edit == 2) {
    if ($arg_hen_id == 0) {
        $arg_hen_id = get_seq ();
        query ("insert into hens (hen_id, coop_id) values (?,?)", 
            array ($arg_hen_id, $coop_id));
    }
    query ("update hens set hen_name = ? where hen_id = ?",
        array ($arg_hen_name, $arg_hen_id));
    $t = sprintf ("coop.php?coop_id=%d", $coop_id);
    redirect ($t);
}

if ($arg_delete == 1) {
    $args = array ();
    $args['delete'] = 2;
    $args['hen_id'] = $arg_hen_id;
    $body .= make_confirm ("Confirm?", "delete", $args);
    pfinish ();
}

if ($arg_delete == 2) {
    query ("delete from hens where hen_id = ?", $arg_hen_id);
    $t = sprintf ("coop.php?coop_id=%d", $coop_id);
    redirect ($t);
}

$body .= "<table class='twocol'>\n";
$body .= "<tr><th>Name</th><td>";
$body .= h($db_hen_name);
$body .= "</td></tr>\n";
$body .= "<tr><th>Key (text)</th><td>";
$body .= h($db_key_text);
$body .= "</td></tr>\n";
$body .= "<tr><th>Key (hex)</th><td>";
$body .= h($db_key);
$body .= "</td></tr>\n";

$body .= "<tr><th></th><td>";
$t = sprintf ("setkey.php?hen_id=%d&edit=1", $arg_hen_id);
$body .= mklink ("change key", $t);
$body .= "</td></tr>\n";

$body .= "</table>\n";

$t = sprintf ("api.php?hen_id=%d", $arg_hen_id);
$t = make_absolute ($t);
$body .= sprintf ("<input type='text' readonly='readonly'"
    ." size='50' value='%s' />\n",
    h($t));
$body .= mklink ("[link]", $t);

pfinish();
    