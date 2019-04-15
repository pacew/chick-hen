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

if ($arg_hen_id) {
    $q = query ("select coop_id, hen_name"
        ." from hens"
        ." where hen_id = ?",
        $arg_hen_id);
    if (($r = fetch ($q)) != NULL) {
        $coop_id = intval ($r->coop_id);
        $db_hen_name = trim ($r->hen_name);
    }
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

pfinish();
    