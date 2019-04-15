<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

$ret = (object)NULL;
$ret->foo = "bar";

json_finish ($ret);
