<?php

require_once ($_SERVER['APP_ROOT'] . "/app.php");

pstart ();

$arg_clear_reports = intval (@$_REQUEST['clear_reports']);
$arg_clear_dangling_chicks = intval (@$_REQUEST['clear_dangling_chicks']);

if ($arg_clear_reports == 1) {
    query ("delete from reported_chicks");
    redirect ("setup.php");
}

if ($arg_clear_dangling_chicks == 1) {
    query ("delete from chicks c"
           ."  where c.hen_id not in (select h.hen_id from hens h)");
    redirect ("setup.php");
}

$body .= "<div>\n";
$body .= mklink ("clear reports", "setup.php?clear_reports=1");
$body .= "</div>\n";

$reported_hens = array ();
$reported_chick_macs = array ();
$reported_chicks = array ();

$q = query ("select hen_name, chick_mac"
            ." from reported_chicks");
while (($r = fetch ($q)) != NULL) {
    $hen_name = trim ($r->hen_name);
    $chick_mac = trim ($r->chick_mac);

    $reported_hens[$hen_name] = 1;
    $reported_chick_macs[$chick_mac] = 1;
    $reported_chicks[] = array ($hen_name, $chick_mac);
}

$hens = array ();
$hens_by_hen_id = array ();
$hens_by_hen_name = array ();
$q = query ("select hen_id, hen_name"
            ." from hens");
while (($r = fetch ($q)) != NULL) {
    $hp = (object)NULL;
    $hp->hen_id = intval ($r->hen_id);
    $hp->hen_name = trim ($r->hen_name);
    $hp->chicks = array ();
    $hp->chicks_by_mac = array ();

    $hens[] = $hp;
    $hens_by_hen_id[$hp->hen_id] = $hp;
    $hens_by_hen_name[$hp->hen_name] = $hp;
    
}

$chicks = array ();
$q = query ("select chick_mac, chick_name, hen_id"
            ." from chicks");
while (($r = fetch ($q)) != NULL) {
    $cp = (object)NULL;
    $cp->chick_mac = trim ($r->chick_mac);
    $cp->chick_name = trim ($r->chick_name);
    $cp->hen_id = intval ($r->hen_id);
    $cp->hen = NULL;

    $chicks[] = $cp;
}

$dangling_chicks = array ();

foreach ($chicks as $cp) {
    if (($hp = @$hens_by_hen_id[$cp->hen_id]) == NULL) {
        $dangling_chicks[] = $cp;
    } else {
        $cp->hen = $hp;
        $hp->chicks[] = $cp;
        
        if ($cp->chick_mac)
            $hp->chicks_by_mac[$cp->chick_mac] = $cp;
    }
}

if (count($dangling_chicks) > 0) {
    $body .= "<h1>dangling chicks</h1>\n";
    foreach ($dangling_chicks as $cp) {
        $body .= sprintf ("%s(%s) ", h($cp->chick_name), h($cp->chick_mac));
    }
    $body .= "<div>\n";
    $body .= mklink ("clear dangling chicks", 
                     "setup.php?clear_dangling_chicks=1");
    $body .= "</div>\n";
}

$body .= "<h1>checking that all known chicks are assigned to hens ...</h1>\n";

$text = "";
$hen_flag = array();
foreach ($reported_chicks as $rcp) {
    $hen_name = $rcp[0];
    $chick_mac = $rcp[1];

    $hp = @$hens_by_hen_name[$hen_name];

    if ($hp == NULL) {
        if (! @$hen_flag[$hen_name]) {
            $text .= sprintf ("<div>unknown hen %s reported by %s</div>\n",
                              h($hen_name), h($chick_mac));
            $hen_flag[$hen_name] = 1;
        }
    } else {
        $cp = @$hp->chicks_by_mac[$chick_mac];
        if ($cp == NULL) {
            $text .= sprintf ("<div>new chick %s reported in for hen %s ",
                              h($chick_mac), h($hen_name));
            $t = sprintf ("chick.php?attach=1"
                          ."&chick_mac=%s"
                          ."&hen_id=%d",
                          urlencode($chick_mac),
                          $hp->hen_id);
            $text .= mklink ("attach?", $t);
            $text .= "</div>\n";
        }
    }
}

if ($text == "") {
    $body .= "<div>... no problems found</div>\n";
} else {
    $body .= $text;
}


pfinish();
    
