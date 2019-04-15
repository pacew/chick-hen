<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_edit = intval(@$_REQUEST['edit']);
$arg_delete = intval(@$_REQUEST['delete']);
$arg_coop_id = intval(@$_REQUEST['coop_id']);
$arg_coop_name = trim(@$_REQUEST['coop_name']);

$db_coop_name = "";

if ($arg_coop_id) {
    $q = query ("select coop_name"
        ." from coops"
        ." where coop_id = ?",
        $arg_coop_id);
    if (($r = fetch ($q)) != NULL) {
        $db_coop_name = $r->coop_name;
    }
}

if($db_coop_name) {
    $body .= sprintf ("<h1>coop %s</h1>\n", h($db_coop_name));
}


if ($arg_edit == 1) {
    $body .= "<form action='coop.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='coop_id' value='%d' />\n",
        $arg_coop_id);
    
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>Name</th><td>\n";
    $body .= sprintf ("<input type='text' name='coop_name' value='%s' />\n",
        $db_coop_name);
    $body .= "</td></tr>\n";

    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Save' />\n";
    $body .= mklink ("cancel", "coops.php");
    if ($arg_coop_id) {
        $body .= " | ";
        $t = sprintf ("coop.php?coop_id=%d&delete=1", $arg_coop_id);
        $body .= mklink ("delete", $t);
    }
            
    $body .= "</td></tr>\n";
    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_edit == 2) {
    if ($arg_coop_id == 0) {
        $arg_coop_id = get_seq ();
        query ("insert into coops (coop_id) values (?)", $arg_coop_id);
    }
    query ("update coops set coop_name = ? where coop_id = ?",
        array ($arg_coop_name, $arg_coop_id));
    redirect ("coops.php");
}

if ($arg_delete == 1) {
    $args = array ();
    $args['delete'] = 2;
    $args['coop_id'] = $arg_coop_id;
    $body .= make_confirm ("Confirm?", "delete", $args);
    pfinish ();
}

if ($arg_delete == 2) {
    query ("delete from coops where coop_id = ?", $arg_coop_id);
    redirect ("coops.php");
}

if ($arg_coop_id == 0)
    pfinish ();

$body .= "<div>\n";
$t = sprintf ("hen.php?coop_id=%d&edit=1", $arg_coop_id);
$body .= mklink ("add hen", $t);
$body .= "</div>\n";

$q = query ("select hen_id, hen_name"
    ." from hens"
    ." where coop_id = ?"
    ." order by hen_name",
    $arg_coop_id);
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $cols = array ();
    $t = sprintf ("hen.php?coop_id=%d&hen_id=%d", $arg_coop_id, $r->hen_id);
    $hen_name = trim ($r->hen_name);
    if ($hen_name == "")
        $hen_name = "_";
    $cols[] = mklink ($hen_name, $t);

    $text = "";
    $t = sprintf ("hen.php?coop_id=%d&hen_id=%d&edit=1", 
        $arg_coop_id, $r->hen_id);
    $text .= mklink ("[edit]", $t);

    $cols[] = $text;


    $rows[] = $cols;
}

$body .= mktable (array ("name", "op"), $rows);

pfinish();
    