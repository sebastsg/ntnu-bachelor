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

function array_keys_exist($array, $keys) {
    foreach ($keys as $key) {
        if (!array_key_exists($key, $array)) {
            return false;
        }
    }
    return true;
}

function experience_for_level($level) {
    return $level * $level * 97;
}

function level_for_experience($experience) {
    $level = 1;
    while ($experience >= experience_for_level($level + 1)) {
        $level++;
    }
    return $level;
}

function verify_login($email, $password) {
    $players = execute_sql('select password_hash from player where email = ?', [$email]);
    if (count($players) !== 1) {
        return false;
    }
    $player = $players[0];
    return password_verify($password, $player['password_hash']);
}

function is_email_registered($email) {
    return count(execute_sql('select id from player where email = ?', [$email])) === 1;
}

function is_display_name_taken($display_name) {
    return count(execute_sql('select id from player where display_name = ?', [$display_name])) === 1;
}

function register_player($display_name, $email, $password) {
    if (strlen($password) < 8) {
        return false;
    }
    if (is_email_registered($email)) {
        return false;
    }
    $password_hash = password_hash($password, PASSWORD_DEFAULT);
    call_sql('register_player', [$display_name, $email, $password_hash]);
    return true;
}

function find_top_players() {
    $stats = execute_sql('select display_name, stat_type, experience from player join stat on stat.player_id = player.id');
    $players = [];
    foreach ($stats as $stat) {
        $players[$stat['display_name']][] = ['stat' => $stat['stat_type'], 'level' => level_for_experience($stat['experience'])];
    }
    return $players;
}
