<?php

function template_execute($name, $args = []) {
    ob_start();
    include("../templates/$name.php");
    return ob_get_clean();
}
