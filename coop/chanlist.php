<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

$arg_chanlist_id = intval (@$_REQUEST['chanlist_id']);
$arg_hwchan_id = intval (@$_REQUEST['hwchan_id']);
$arg_hwchan_name = trim (@$_REQUEST['hwchan_name']);
$arg_chan_type = intval (@$_REQUEST['chan_type']);
$arg_port = intval (@$_REQUEST['port']);
$arg_bit_width = intval (@$_REQUEST['bit_width']);
$arg_bit_position = intval (@$_REQUEST['bit_position']);
$arg_edit = intval (@$_REQUEST['edit']);
$arg_delete = intval (@$_REQUEST['delete']);
$arg_move = intval (@$_REQUEST['move']);
$arg_edit_name = intval (@$_REQUEST['edit_name']);
$arg_chanlist_name = trim (@$_REQUEST['chanlist_name']);

pstart ();

$q = query ("select chanlist_name"
            ." from chanlists"
            ." where chanlist_id = ?",
            $arg_chanlist_id);
if (($r = fetch ($q)) == NULL) {
    $body .= "not found";
    pfinish();
}

$chanlist_name = trim ($r->chanlist_name);

$body .= sprintf ("<h1>chanlist: %s</h1>\n", $chanlist_name);

if ($arg_edit_name == 1) {
    $body .= "<form action='chanlist.php'>\n";
    $body .= "<input type='hidden' name='edit_name' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='chanlist_id' value='%d' />\n",
                      $arg_chanlist_id);
    $body .= "New name: ";
    $body .= "<input type='text' name='chanlist_name' />\n";
    $body .= "<input type='submit' value='Save' />\n";
    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    $body .= mklink ("cancel", $t);
    $body .= "</form>\n";
    pfinish ();
}
                      
if ($arg_edit_name == 2) {
    query ("update chanlists set chanlist_name = ? where chanlist_id = ?",
           array ($arg_chanlist_name, $arg_chanlist_id));
    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    redirect ($t);
}

$body .= "<div>\n";
$t = sprintf ("chanlist.php?edit_name=1&chanlist_id=%d", $arg_chanlist_id);
$body .= mklink ("rename", $t);
$body .= "</div>\n";


if ($arg_hwchan_id) {
    $q = query ("select hwchan_name, chan_type, port,"
                ."  bit_width, bit_position"
                ." from hwchan"
                ." where hwchan_id = ?",
                $arg_hwchan_id);
    if (($r = fetch ($q)) == NULL)
        fatal ("not found");

    $db_hwchan_name = trim ($r->hwchan_name);
    $db_chan_type = intval ($r->chan_type);
    $db_port = intval ($r->port);
    $db_bit_width = intval ($r->bit_width);
    $db_bit_position = intval ($r->bit_position);
} else {
    $db_hwchan_name = "";
    $db_chan_type = 0;
    $db_port = 0;
    $db_bit_width = 0;
    $db_bit_position = 0;
}

if ($arg_edit == 1) {
    $body .= "<h1>edit hardware channel</h1>\n";
    $body .= "<form action='chanlist.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='chanlist_id' value='%d' />\n",
                      $arg_chanlist_id);
    $body .= sprintf ("<input type='hidden' name='hwchan_id' value='%d' />\n",
                      $arg_hwchan_id);
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>Name</th><td>\n";
    $body .= sprintf ("<input type='text' name='hwchan_name' value='%s' />\n",
                      h($db_hwchan_name));
    $body .= "</td></tr>\n";
    $body .= "<tr><th>Channel type</th><td>\n";
    $body .= sprintf ("<input type='text' name='chan_type' value='%s' />\n",
                      $db_chan_type ? $db_chan_type : "");
    $body .= "</td></tr>\n";
    $body .= "<tr><th>Hardware port</th><td>\n";
    $body .= sprintf ("<input type='text' name='port' value='%s' />\n",
                      $db_port);
    $body .= "</td></tr>\n";
    $body .= "<tr><th>Bit width</th><td>\n";
    $body .= sprintf ("<input type='text' name='bit_width' value='%s' />\n",
                      $db_bit_width);
    $body .= "</td></tr>\n";
    $body .= "<tr><th>Bit position</th><td>\n";
    $body .= sprintf ("<input type='text' name='bit_position' value='%s' />\n",
                      $db_bit_position);
    $body .= "</td></tr>\n";
    $body .= "<tr><th></th><td>";
    $body .= "<input type='submit' value='Save' />\n";
    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    $body .= mklink ("cancel", $t);
    $body .= " | ";
    $t = sprintf ("chanlist.php?chanlist_id=%d&hwchan_id=%d&delete=1",
                  $arg_chanlist_id, $arg_hwchan_id);
    $body .= mklink ("delete", $t);

    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish();
    
}

if ($arg_edit == 2) {
    if ($arg_hwchan_id == 0) {
        $q = query ("select max(sort_order) as sort_order"
                    ." from hwchan"
                    ." where chanlist_id = ?",
                    $arg_chanlist_id);
        $r = fetch ($q);
        $sort_order = $r->sort_order + 1;

        $arg_hwchan_id = get_seq ();
        query ("insert into hwchan ("
               ." hwchan_id, chanlist_id, sort_order, hwchan_name,"
               ." chan_type, port, bit_width, bit_position"
               .") values (?,?,?,?,?,?,?,?)",
               array ($arg_hwchan_id, $arg_chanlist_id, $sort_order,
                      $arg_hwchan_name, $arg_chan_type, $arg_port,
                      $arg_bit_width, $arg_bit_position));
    } else {
        query ("update hwchan set chanlist_id = ?, hwchan_name = ?,"
               ."  chan_type =?, port = ?, bit_width = ?, bit_position = ?"
               ." where hwchan_id = ?",
               array ($arg_chanlist_id, $arg_hwchan_name, $arg_chan_type,
                      $arg_port, $arg_bit_width, $arg_bit_position,
                      $arg_hwchan_id));
    }

    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    redirect ($t);
}

if ($arg_delete == 1) {
    $args = array ();
    $args['delete'] = 2;
    $args['chanlist_id'] = $arg_chanlist_id;
    $args['hwchan_id'] = $arg_hwchan_id;
    $body .= make_confirm ("Confirm?", "delete", $args);
    pfinish();
}

if ($arg_delete == 2) {
    query ("delete from hwchan where hwchan_id = ?", $arg_hwchan_id);
    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    redirect ($t);
}

if ($arg_move) {

    $q = query ("select hwchan_id"
                ." from hwchan"
                ." where chanlist_id = ?"
                ." order by sort_order, hwchan_id",
                $arg_chanlist_id);
    $ids = array ();
    $idx = -1;
    while (($r = fetch ($q)) != NULL) {
        $hwchan_id = intval ($r->hwchan_id);
        if ($hwchan_id == $arg_hwchan_id)
            $idx = count($ids);
        $ids[] = $hwchan_id;
    }

    if ($arg_move == -1) {
        if ($idx > 0) {
            $temp = $ids[$idx - 1];
            $ids[$idx - 1] = $ids[$idx];
            $ids[$idx] = $temp;
        }
    } else if ($arg_move == 1) {
        if ($idx + 1 < count($ids)) {
            $temp = $ids[$idx];
            $ids[$idx] = $ids[$idx + 1];
            $ids[$idx + 1] = $temp;
        }
    }

    for ($idx = 0; $idx < count($ids); $idx++) {
        query ("update hwchan set sort_order = ? where hwchan_id = ?",
               array ($idx, $ids[$idx]));
    }

    $t = sprintf ("chanlist.php?chanlist_id=%d", $arg_chanlist_id);
    redirect ($t);
}


$q = query ("select hwchan_id, hwchan_name, chan_type, port,"
            ."  bit_width, bit_position"
            ." from hwchan"
            ." where chanlist_id = ?"
            ." order by sort_order, hwchan_id",
            $arg_chanlist_id);
$rows = array ();
while (($r = fetch ($q)) != NULL) {
    $cols = array ();

    $t = sprintf ("chanlist.php?chanlist_id=%d&hwchan_id=%d&edit=1",
                  $arg_chanlist_id, $r->hwchan_id);
    $cols[] = mklink ($r->hwchan_name, $t);
    $cols[] = mklink ($r->chan_type ? $r->chan_type : "", $t);
    $cols[] = mklink (intval($r->port), $t);
    $cols[] = mklink (intval($r->bit_width), $t);
    $cols[] = mklink (intval($r->bit_position), $t);
    
    $text = mklink ("[edit]", $t);
    $text .= " &nbsp;&nbsp; ";
    $t = sprintf ("chanlist.php?chanlist_id=%d&hwchan_id=%d&move=-1",
                  $arg_chanlist_id, $r->hwchan_id);
    $text .= mklink ("[move up]", $t);
    $text .= " &nbsp;&nbsp; ";
    $t = sprintf ("chanlist.php?chanlist_id=%d&hwchan_id=%d&move=1",
                  $arg_chanlist_id, $r->hwchan_id);
    $text .= mklink ("[move down]", $t);
    $cols[] = $text;

    $rows[] = $cols;
}

$body .= mktable (array ("name", "type", "port", "width", "position", "op"),
                  $rows);

$t = sprintf ("chanlist.php?chanlist_id=%d&edit=1", $arg_chanlist_id);
$body .= mklink ("add channel", $t);
pfinish();
    
