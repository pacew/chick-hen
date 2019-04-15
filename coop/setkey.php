<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$body .= "<h1>setkey</h1>\n";

$arg_edit = intval(@$_REQUEST['edit']);
$arg_hen_id = intval(@$_REQUEST['hen_id']);
$arg_key = trim (@$_REQUEST['key']);

$db_hen_name = "";

if ($arg_hen_id == 0)
    fatal ("invalid argument");

if ($arg_edit == 1) {
    $body .= "<form action='setkey.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
        $arg_hen_id);
    
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>New key</th><td>\n";
    $body .= "<input type='text' name='key' />\n";
    $body .= "</td></tr>\n";

    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Update' />\n";
    
    $t = sprintf ("hen.php?hen_id=%d", $arg_hen_id);
    $body .= mklink ("cancel", $t);

    $body .= "</td></tr>\n";
    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_edit == 2) {
    $salt = "22906";
    $h = hash_pbkdf2("sha256",$arg_key,$salt,100000);
    query ("update hens set key_text = ?, key = ? where hen_id = ?",
        array ($arg_key, $h, $arg_hen_id));
    $t = sprintf ("hen.php?hen_id=%d", $arg_hen_id);
    redirect ($t);
}
