<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_edit = intval(@$_REQUEST['edit']);
$arg_delete = intval(@$_REQUEST['delete']);
$arg_hen_id = intval(@$_REQUEST['hen_id']);
$arg_hen_name = trim(@$_REQUEST['hen_name']);

$db_hen_name = "";
$db_key_text = "";
$db_key = "";

if ($arg_hen_id) {
    $q = query ("select hen_name, key_text, key"
        ." from hens"
        ." where hen_id = ?",
        $arg_hen_id);
    if (($r = fetch ($q)) != NULL) {
        $db_hen_name = trim ($r->hen_name);
        $db_key_text = trim ($r->key_text);
        $db_key = trim ($r->key);
    }
}

$body .= sprintf ("<h1>hen %s</h1>\n", h($db_hen_name));


if ($arg_edit == 1) {
    $body .= "<form action='hen.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
        $arg_hen_id);
    
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>Name</th><td>\n";
    $body .= sprintf ("<input type='text' name='hen_name' value='%s' />\n",
        $db_hen_name);
    $body .= "</td></tr>\n";

    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Save' />\n";
    $body .= mklink ("cancel", "hens.php");
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
        query ("insert into hens (hen_id) values (?)", 
            array ($arg_hen_id));
    }
    query ("update hens set hen_name = ? where hen_id = ?",
        array ($arg_hen_name, $arg_hen_id));
    redirect ("hens.php");
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
    redirect ("hens.php");
}

$body .= "<table class='twocol'>\n";
$body .= "<tr><th>ID</th><td>";
$body .= sprintf ("%d", $arg_hen_id);
$body .= "</td></tr>\n";
$body .= "<tr><th>Name</th><td>";
$body .= h($db_hen_name);
$body .= " ";
$t = sprintf ("hen.php?hen_id=%d&edit=1", $arg_hen_id);
$body .= mklink ("[edit]", $t);
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

$q = query ("select chick_name, chick_mac"
            ." from chicks"
            ." where hen_id = ?"
            ." order by chick_name, chick_mac",
            $arg_hen_id);
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $cols = array ();

    $t = sprintf ("chick.php?chick_mac=%s&hen_id=%d", 
                  urlencode ($r->chick_mac), $arg_hen_id);

    $cols[] = mklink ($r->chick_name, $t);
    $cols[] = mklink ($r->chick_mac, $t);

    $rows[] = $cols;
}

$body .= "<h1>chicks</h1>\n";
$body .= mktable (array ("name", "mac"), $rows);
$t =sprintf ("chick.php?edit=1&hen_id=%d", $arg_hen_id);
$body .= "<div>\n";
$body .= mklink ("add chick", $t);
$body .= "</div>\n";            

pfinish();
    
