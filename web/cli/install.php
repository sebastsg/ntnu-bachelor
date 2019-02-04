<?php

require_once('../source/init.php');

$sql_files = get_files_in_directory(DATABASE_SCRIPTS_DIRECTORY);
foreach ($sql_files as $sql_file) {
    echo "$sql_file: ";
    $success = execute_sql_file(DATABASE_SCRIPTS_DIRECTORY . '/' . $sql_file);
    echo ($success ? "success\n" : "failure\n");
}
