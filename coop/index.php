<?php
require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$body .= mklink ("home", "/");
$body .= " | ";
$body .= mklink ("coops", "coops.php");

pfinish();
    
