<?php

define('ROUTER_STATUS_NOT_EXECUTED',  0);
define('ROUTER_STATUS_OKAY',          1);
define('ROUTER_STATUS_NO_ACTION',     2);

$router_root = '';
$router_nodes = [];
$router_bind_route = [];
$router_status = ROUTER_STATUS_NOT_EXECUTED;

function get_router_status() {
    global $router_status;
    return $router_status;
}

function router_route_to_string() {
    global $router_root, $router_nodes;
    return $router_root . ':' . implode('/', $router_nodes);
}

function router_is_variable($key) {
    if (strlen($key) === 0) {
        return false;
    }
    return $key[0] === '{' && $key[strlen($key) - 1] === '}';
}

function router_parse_uri() {
    $base = implode('/', array_slice(explode('/', $_SERVER['SCRIPT_NAME']), 0, -1)) . '/';
    $uri = substr($_SERVER['REQUEST_URI'], strlen($base));
    if (strstr($uri, '?')) {
        $uri = substr($uri, 0, strpos($uri, '?'));
    }
    $uri = '/' . trim($uri, '/');
    return $uri;
}

function router_parse() {
    global $router_nodes, $router_root;
    $uri = router_parse_uri();
    $router_nodes = explode('/', $uri);
    $router_root = $router_nodes[1];
    array_shift($router_nodes);
    array_shift($router_nodes);
}

function router_bind($path, $action) {
    global $router_bind_route;
    $path = substr($path, 1); // Trim the slash.
    $parts = explode('/', $path);
    $current = &$router_bind_route;
    foreach ($parts as $part) {
        if (router_is_variable($part)) {
            $part = '{?}';
        }
        if (!isset($current[$part])) {
            $current[$part] = [];
        }
        $current = &$current[$part];
    }
    $current['/'] = [
        'action' => $action
    ];
}

function router_bind_pages($ajax_prefix, $pages) {
    foreach ($pages as $page) {
        router_bind('/' . $page, function() use ($page) {
            return template_execute('main', [
                'page' => template_execute($page)
            ]);
        });
        router_bind("/$ajax_prefix/$page", function() use ($page) {
            return template_execute($page);
        });
    }
}

function router_execute() {
    global $router_bind_route, $router_root, $router_nodes, $router_status;
    router_parse();
    $params = array_merge([ $router_root ], $router_nodes);
    $current = $router_bind_route;
    $args = [];
    for ($i = 0; $i < count($params); $i++) {
        if (isset($current['action']) && is_callable($current['action'])) {
            break;
        }
        $keys = array_keys($current);
        if (!$keys) {
            $router_status = ROUTER_STATUS_NO_ACTION;
            return false;
        }
        // First check if there is a matching non-variable
        $found = false;
        foreach ($keys as $key) {
            if (!router_is_variable($key) && $key == $params[$i]) {
                $current = &$current[$key];
                $found = true;
                break;
            }
        }
        // If not, just assume it's a variable:
        if (!$found) {
            if (!isset($current['{?}'])) {
                $router_status = ROUTER_STATUS_NO_ACTION;
                return false;
            }
            $current = &$current['{?}'];
            $args[] = $params[$i];
        }
    }
    $current = $current['/'];
    if (!$current || !is_callable($current['action'])) {
        echo '<pre class="box">Not callable</pre>';
        $router_status = ROUTER_STATUS_NO_ACTION;
        return false;
    }
    $router_status = ROUTER_STATUS_OKAY;
    $output = call_user_func_array($current['action'], $args);
    if (is_array($output)) {
        echo json_encode($output);
    } else {
        echo $output;
    }
}
