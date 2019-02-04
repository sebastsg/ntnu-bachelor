<?php

header('Content-Type: text/html; charset=utf-8');
header('X-XSS-Protection: 1; mode=block');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');

session_start();

require_once('../config.php');
require_once('router.php');
require_once('files.php');
require_once('template.php');
require_once('database.php');
