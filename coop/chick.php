<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_attach = intval (@$_REQUEST['attach']);
$arg_chick_mac = trim (@$_REQUEST['chick_mac']);
$arg_hen_id = intval (@$_REQUEST['hen_id']);
$arg_edit = intval (@$_REQUEST['edit']);
$arg_chick_name = trim (@$_REQUEST['chick_name']);
$arg_delete = intval (@$_REQUEST['delete']);
$arg_selected_chick_mac = trim (@$_REQUEST['selected_chick_mac']);
$arg_assign_chanlist = intval (@$_REQUEST['assign_chanlist']);
$arg_chanlist_id = intval (@$_REQUEST['chanlist_id']);

if ($arg_attach == 1) {
    $possibilities = array ();
    $q = query ("select chick_mac, chick_name"
                ." from chicks"
                ." where hen_id = ?"
                ."   and chick_mac like 'placeholder%'",
                $arg_hen_id);
    while (($r = fetch ($q)) != NULL) {
        $possibilities[] = array ($r->chick_mac, $r->chick_name);
    }

    if (count ($possibilities) == 0) {
        attach_chick ($arg_chick_mac, $arg_hen_id);
        redirect ("setup.php");
    }

    $body .= "<form action='chick.php'>\n";
    $body .= "<input type='hidden' name='attach' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
                      $arg_hen_id);
    $body .= sprintf ("<input type='hidden' name='chick_mac'"
                      ." value='%s' />\n", h($arg_chick_mac));
    $body .= "<div>Attach where?</div>\n";
    $body .= "<select name='selected_chick_mac'>\n";
    $body .= "<option value=''></option>\n";
    foreach ($possibilities as $poss) {
        $chick_mac = $poss[0];
        $chick_name = $poss[1];
        
        if (($text = trim ($chick_name)) == "")
            $text = $chick_mac;

        $body .= sprintf ("<option value='%s'>%s</option>\n",
                          h($chick_mac), h($text));
    }
    $body .= "<option value='new'>create new</option>\n";
    $body .= "</select>\n";
    $body .= "<input type='submit' value='Save' />\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_attach == 2) {
    if ($arg_selected_chick_mac == "") {
        $t = sprintf ("chick.php?attach=1&chick_mac=%s&hen_id=%d",
                      urlencode($arg_chick_mac), $arg_hen_id);
        redirect ($t);
    }

    if ($arg_selected_chick_mac == "new") {
        attach_chick ($arg_chick_mac, $arg_hen_id);
        redirect ("setup.php");
    }

    query ("update chicks set chick_mac = ?, hen_id = ?"
           ." where chick_mac = ?",
           array ($arg_chick_mac, $arg_hen_id, $arg_selected_chick_mac));

    redirect ("setup.php");
}

function attach_chick ($chick_mac, $hen_id) {
    $q = query ("select hen_id"
                ." from chicks"
                ." where chick_mac = ?",
                $chick_mac);
    if (($r = fetch ($q)) == NULL) {
        query ("insert into chicks (chick_mac, hen_id) values (?,?)",
               array ($chick_mac, $hen_id));
    } else {
        query ("update chicks set hen_id = ? where chick_mac = ?",
               array ($hen_id, $chick_mac));
    }
}

if ($arg_chick_mac) {
    $q = query ("select chick_name, hen_id, chanlist_id"
                ." from chicks"
                ." where chick_mac = ?",
                $arg_chick_mac);
    if (($r = fetch ($q)) == NULL) {
        $body .= "not found";
        pfinish ();
    }

    $db_chick_name = trim ($r->chick_name);
    $db_hen_id = intval ($r->hen_id);
    $db_chanlist_id = intval ($r->chanlist_id);

    $q = query ("select hen_name"
                ." from hens"
                ." where hen_id = ?",
                $db_hen_id);
    if (($r = fetch ($q)) == NULL) {
        $db_hen_name = "";
    } else {
        $db_hen_name = trim ($r->hen_name);
    }
} else {
    $db_chick_name = "";
    $db_hen_id = 0;
    $db_hen_name = "";
    $db_chanlist_id = 0;
}

$db_chanlist_name = "";
if ($db_chanlist_id) {
    $q = query ("select chanlist_name"
                ." from chanlists"
                ." where chanlist_id = ?",
                $db_chanlist_id);
    if (($r = fetch ($q)) != NULL) {
        $db_chanlist_name = trim ($r->chanlist_name);
    }
}

if ($arg_hen_id && $db_hen_id && $arg_hen_id != $db_hen_id) {
    $fatal ("inconsistent hen_id");
}

if ($arg_edit == 1) {
    if ($arg_chick_mac) {
        $body .= sprintf ("<h2>chick mac %s</h2>\n", h($arg_chick_mac));
    }
    $body .= "<form action='chick.php'>\n";
    $body .= "<input type='hidden' name='edit' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='chick_mac' value='%s' />\n",
                      h($arg_chick_mac));
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
                      $arg_hen_id);
    $body .= "<table class='twocol'>\n";
    $body .= "<tr><th>chick name</th><td>\n";
    $body .= sprintf ("<input type='text' name='chick_name' value='%s' />\n",
                      $db_chick_name);
    $body .= "</td></tr>\n";
    $body .= "<tr><th></th><td>\n";
    $body .= "<input type='submit' value='Save' />\n";
    if ($arg_hen_id) {
        $t = sprintf ("hen.php?hen_id=%d", $arg_hen_id);
    } else if ($arg_chick_mac) {
        $t = sprintf ("chick.php?chick_mac=%s", h($arg_chick_mac));
    } else {
        $t = "hens.php";
    }
    $body .= mklink ("cancel", $t);
    $body .= "</td></tr>\n";
    $body .= "</table>\n";
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_edit == 2) {
    if ($arg_chick_mac) {
        query ("update chicks set chick_name = ? where chick_mac = ?",
               array ($arg_chick_name, $arg_chick_mac));
        $t = sprintf ("chick.php?chick_mac=%s&hen_id=%d",
                      urlencode ($arg_chick_mac), $arg_hen_id);
        redirect ($t);
    }

    if ($arg_chick_name == "")
        fatal ("chick name can't be empty");
        
    if ($arg_hen_id == 0)
        fatal ("need hen to create chick");

    $mac = sprintf ("placeholder%d", get_seq());
    query ("insert into chicks (chick_mac, chick_name, hen_id)"
           ." values (?,?,?)",
           array ($mac, $arg_chick_name, $arg_hen_id));

    $t = sprintf ("hen.php?hen_id=%d", $arg_hen_id);
    redirect ($t);
}

if ($arg_delete == 1) {
    $args = array ();
    $args['delete'] = 2;
    $args['chick_mac'] = $arg_chick_mac;
    $args['hen_id'] = $arg_hen_id;
    $body .= make_confirm ("Confirm?", "delete", $args);
    pfinish();
}

if ($arg_delete == 2) {
    query ("delete from chicks where chick_mac = ?", $arg_chick_mac);
    $t = sprintf ("hen.php?hen_id=%d", $arg_hen_id);
    redirect ($t);
}

if ($arg_assign_chanlist == 1) {
    $q = query ("select chanlist_id, chanlist_name"
                ." from chanlists"
                ." order by chanlist_name, chanlist_id");
    $clist = array ();
    while (($r = fetch ($q)) != NULL) {
        $elt = (object)NULL;
        $elt->chanlist_id = intval ($r->chanlist_id);
        $elt->chanlist_name = trim ($r->chanlist_name);
        $clist[] = $elt;
    }

    $body .= "<h1>assign chanlist</h1>\n";
    $body .= "<form action='chick.php'>\n";
    $body .= "<input type='hidden' name='assign_chanlist' value='2' />\n";
    $body .= sprintf ("<input type='hidden' name='chick_mac' value='%s' />\n",
                      h($arg_chick_mac));
    $body .= sprintf ("<input type='hidden' name='hen_id' value='%d' />\n",
                      $arg_hen_id);
    $body .= "<select name='chanlist_id'>\n";
    make_option (0, $db_chanlist_id, "--select--");
    foreach ($clist as $elt) {
        make_option ($elt->chanlist_id, $db_chanlist_id, $elt->chanlist_name);
    }
    $body .= "</select>\n";
    $body .= "<input type='submit' value='Save' />\n";
    $t = sprintf ("chick.php?chick_mac=%s&hen_id=%d",
                  urlencode ($arg_chick_mac), $arg_hen_id);
    $body .= mklink ("cancel", $t);
    $body .= "</form>\n";
    pfinish ();
}

if ($arg_assign_chanlist == 2) {
    query ("update chicks set chanlist_id = ? where chick_mac = ?",
           array ($arg_chanlist_id, $arg_chick_mac));
    $t = sprintf ("chick.php?chick_mac=%s&hen_id=%d",
                  urlencode ($arg_chick_mac), $arg_hen_id);
    redirect ($t);
}

$body .= "<table class='twocol'>\n";
$body .= "<tr><th>chick_mac</th><td>";
$body .= h($arg_chick_mac);
$t = sprintf ("chick.php?delete=1&chick_mac=%s&hen_id=%d",
              urlencode ($arg_chick_mac), $arg_hen_id);
$body .= " ";
$body .= mklink ("[delete]", $t);
$body .= "</td></tr>\n";
$body .= "<tr><th>chick_name</th><td>";
$body .= h($db_chick_name);
$t = sprintf ("chick.php?chick_mac=%s&edit=1", urlencode ($arg_chick_mac));
$body .= " ";
$body .= mklink ("[edit]", $t);
$body .= "</td></tr>\n";
$body .= "<tr><th>hen_name</th><td>";
$t = sprintf ("hen.php?hen_id=%d", $db_hen_id);
$body .= mklink($db_hen_name, $t);
$body .= "</td></tr>\n";
$body .= "<tr><th>chanlist</th><td>\n";
$t = sprintf ("chanlist.php?chanlist_id=%d", $db_chanlist_id);
$body .= mklink ($db_chanlist_name, $t);
$t = sprintf ("chick.php?chick_mac=%s&hen_id=%d&assign_chanlist=1",
              urlencode ($arg_chick_mac), $arg_hen_id);
$body .= " ";
$body .= mklink ("[change]", $t);
$body .= "</td></tr>\n";


$body .= "</table>\n";

pfinish();
    
