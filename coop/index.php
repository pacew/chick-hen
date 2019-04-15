<?php
var_dump ($_REQUEST);

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$body .= mklink ("coops", "coops.php");

pfinish();
    