<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

$arg_edit = intval (@$_REQUEST['edit']);
$arg_chanlist_id = intval (@$_REQUEST['chanlist_id']);
$arg_chanlist_name = trim (@$_REQUEST['chanlist_name']);

pstart ();

$db_chanlist_name = "";

if ($arg_chanlist_id) {
    $q = query ("select chanlist_name"
                ." from chanlists"
                ." where chanlist_id = ?",
                $arg_chanlist_id);
    if (($r = fetch ($q)) != NULL) {
        $db_chanlist_name = $r->name;
    }
}

if ($arg_edit == 1) {
    $body .= "<form action='chans.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='chanlist_id' value='%d' />\n",
                      $arg_chanlist_id);
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>chanlist name</th><td>";
    $body .= sprintf ("<input type='text' name='chanlist_name' value='%s' '>\n",
                      h($db_chanlist_name));
    $body .= "</td></tr>\n";
    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Save' />\n";
    $body .= mklink ("cancel", "chans.php");
    $body .= "</td></tr>\n";
    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_edit == 2) {
    if ($arg_chanlist_id == 0) {
        $arg_chanlist_id = get_seq ();
        query ("insert into chanlists (chanlist_id, chanlist_name)"
               ." values (?,?)",
               array ($arg_chanlist_id, $arg_chanlist_name));
    } else {
        query ("update chanlists set chanlist_name = ? where chanlist_id = ?",
               array ($arg_chanlist_name, $arg_chanlist_id));
    }
    redirect ("chans.php");
}

$q = query ("select hwchan_id, chanlist_id, hwchan_name"
            ." from hwchan"
            ." order by sort_order, hwchan_id");
$chans = array ();
while (($r = fetch ($q)) != NULL) {
    $hwchan_id = intval ($r->hwchan_id);
    $chanlist_id = intval ($r->chanlist_id);
    $hwchan_name = trim ($r->hwchan_name);

    if (! isset ($chans[$chanlist_id]))
        $chans[$chanlist_id] = array ();
    $chans[$chanlist_id][] = $hwchan_name;
}
            
$q = query ("select chanlist_id, chanlist_name"
            ." from chanlists"
            ." order by chanlist_name, chanlist_id");
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $chanlist_id = intval ($r->chanlist_id);
    $chanlist_name = trim ($r->chanlist_name);

    $cols = array ();
    $t = sprintf ("chanlist.php?chanlist_id=%d", $chanlist_id);
    $cols[] = mklink ($chanlist_name, $t);

    $text = "";
    if (isset ($chans[$chanlist_id])) {
        foreach ($chans[$chanlist_id] as $name) {
            $text .= h($name);
            $text .= " ";
        }
    }
    $cols[] = $text;

    $rows[] = $cols;
}

$body .= mktable (array ("name", "channels"), $rows);

$body .= mklink ("add chanlist", "chans.php?edit=1");

pfinish();
    
