<?php

require_once('../source/init.php');

router_bind('/', function() {
    return template_execute('main', [
        'page' => template_execute('news')
    ]);
});

router_bind('/page', function() {
    return template_execute('news');
});

router_bind_pages('page', ['news', 'signup', 'leaderboard', 'world']);

router_bind('/api/verify/{email}/{password}', function($email, $password) {
    // todo: protection against mass attempts
    return ['is_valid' => verify_login($email, $password)];
});

router_bind('/api/email_registered/{email}', function($email) {
    return ['is_taken' => is_email_registered($email)];
});

router_bind('/api/name_taken/{name}', function($display_name) {
    return ['is_taken' => is_display_name_taken($display_name)];
});

router_bind('/post/signup', function() {
    if (!array_keys_exist($_POST, ['display_name', 'email', 'password'])) {
        return ['status' => false];
    }
    $status = register_account($_POST['email'], $_POST['password']);
    $status = $status && register_player($_POST['email'], $_POST['display_name']);
    return ['status' => $status];
});

router_execute();
