<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_edit = intval(@$_REQUEST['edit']);
$arg_delete = intval(@$_REQUEST['delete']);

$body .= "<div>\n";
$body .= mklink ("add hen", "hen.php?edit=1");
$body .= "</div>\n";

$q = query ("select hen_id, hen_name"
    ." from hens"
    ." order by hen_name");
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $cols = array ();
    $t = sprintf ("hen.php?hen_id=%d", $r->hen_id);
    $hen_name = trim ($r->hen_name);
    if ($hen_name == "")
        $hen_name = "_";
    $cols[] = mklink ($hen_name, $t);

    $text = "";
    $t = sprintf ("hen.php?hen_id=%d&edit=1", 
                  $r->hen_id);
    $text .= mklink ("[edit]", $t);

    $cols[] = $text;


    $rows[] = $cols;
}

$body .= mktable (array ("name", "op"), $rows);

pfinish();
    
