<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$body .= "<h1>coops</h1>\n";
$body .= "<div>\n";
$body .= mklink ("home", "/");
$body .= "</div>\n";

$q = query ("select coop_id, coop_name"
    ." from coops"
    ." order by coop_name");
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $cols = array ();
    $t = sprintf ("coop.php?coop_id=%d", $r->coop_id);
    $coop_name = trim ($r->coop_name);
    if ($coop_name == "")
        $coop_name = "_";
    $cols[] = mklink ($coop_name, $t);

    $t = sprintf ("coop.php?coop_id=%d&edit=1", $r->coop_id);
    $cols[] = mklink ("[edit]", $t);
    
    $rows[] = $cols;
}

$body .= mktable (array ("name", "op"), $rows);

$body .= mklink ("add coop", "coop.php?edit=1");

pfinish();
