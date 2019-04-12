<?php

$anon_ok = 1;
require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_email = trim (@$_REQUEST['email']);
$arg_passwd = trim (@$_REQUEST['passwd']);

if ($arg_email) {
    $q = query ("select user_id, passwd"
                ." from users"
                ." where email = ?",
                $arg_email);
    $r = fetch ($q);
    if (password_verify ($arg_passwd, $r->passwd)) {
        putsess ("username", $arg_email);
        redirect ("/");
    }

    $body .= "login incorrect";
    pfinish();
}

$body .= "<form action='login.php' method='post'>\n";
$body .= "<table class='twocol'>\n";
$body .= "<tr>\n";
$body .= "<th>Login name</th>\n";
$body .= "<td><input type='text' name='email' /></td>\n";
$body .= "</tr>\n";
$body .= "<tr>\n";
$body .= "<th>Password</th>\n";
$body .= "<td><input type='password' name='passwd' /></td>\n";
$body .= "</tr>\n";
$body .= "<tr><th></th>\n";
$body .= "<td><input type='submit' value='Login' /></td>\n";
$body .= "</tr>\n";
$body .= "</table>\n";
$body .= "</form>\n";

pfinish();
    