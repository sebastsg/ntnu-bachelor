<?php

require_once('../source/init.php');

router_bind('/', function() {
    return template_execute('main', [
        'page' => template_execute('news')
    ]);
});

router_bind('/ajax', function() {
    return template_execute('news');
});

router_bind_pages([
    'news',
    'signup',
    'download',
    'leaderboard',
    'world'
]);

echo router_execute();
