<?php

require_once ($_SERVER['APP_ROOT'] . "/psite.php");

function pstart () {
    psite_session ();

    global $body;
    $body = "";
}

function pfinish() {
    global $body;
    
    $pg = "";

    $pg .= "<!DOCTYPE html>\n"
        ."<html lang='en'>\n"
        ."<head>\n"
        ."<meta charset='utf-8'>\n"
        ."<meta name='viewport' content='width=device-width,initial-scale=1'>\n";
    
    $pg .= "<title>";
    $pg .= "title";
    $pg .= "</title>\n";

    $pg .= sprintf ("<link rel='stylesheet' href='reset.css?c=%s' />\n",
                    get_cache_defeater ());
    $pg .= sprintf ("<link rel='stylesheet' href='style.css?c=%s' />\n",
                    get_cache_defeater ());

    $pg .= "<script src='https://ajax.googleapis.com"
        ."/ajax/libs/jquery/2.1.4/jquery.min.js'></script>\n";

    $pg .= "</head>\n";
    
    $pg .= "<body>\n";

    $pg .= "<div class='content'>\n";
    $pg .= $body;
    $pg .= "</div>\n";

    $pg .= sprintf ("<script src='scripts.js?c=%s'></script>\n",
                    get_cache_defeater ());

    $pg .= "</body>\n";
    $pg .= "</html>\n";
    echo ($pg);

    do_commits ();
}
