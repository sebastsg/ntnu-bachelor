<?php

$db = false;

function connect_database() {
    global $db;
    if ($db !== false) {
        return $db;
    }
    try {
        $dsn = 'pgsql:host=' . DATABASE_HOST . ';dbname=' . DATABASE_NAME;
        $db = new PDO($dsn, DATABASE_USER, DATABASE_PASS);
        $db->exec('set names utf8');
        $db->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
        $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    } catch (PDOException $e) {
        return false;
    } catch (Exception $e) {
        return false;
    }
}

function execute_sql($query, $parameters = [], $single_row = false) {
    $db = connect_database();
    if (!$db) {
        return false;
    }
    try {
        $statement = $db->prepare($query);
        $statement->execute($parameters);
        $result = $statement->fetchAll(PDO::FETCH_ASSOC);
        if (!$result) {
            return [];
        }
        if ($single_row) {
            return $result[0];
        }
        return $result;
    } catch (PDOException $e) {
        echo '<pre>';
        print_r($e);
        echo '</pre>';
        return false;
    } catch (Exception $e) {
        return false;
    }
}

function group_sql_result($rows, $group_by) {
    $result = [];
    foreach ($rows as $row) {
        $result[$row[$group_by]][] = $row;
    }
    return $result;
}

function make_question_marks($count) {
    $questions = '';
    for ($i = 0; $i < $count; $i++) {
        $questions .= '?,';
    }
    if (strlen($questions) > 0) {
        $questions = substr($questions, 0, -1);
    }
    return $questions;
}

function call_sql($procedure, $parameters = [], $single_row = false) {
    $questions = make_question_marks(count($parameters));
    $result = execute_sql("call $procedure ($questions)", $parameters, $single_row);
    if (!$result) {
        return [];
    }
    return $result;
}

function execute_sql_file($path) {
    if (!file_exists($path)) {
        return false;
    }
    $db = connect_database();
    if (!$db) {
        return false;
    }
    $query = file_get_contents($path);
    if (!$query) {
        return false;
    }
    try {
        // temporarily emulate prepares to allow multiple queries
        $db->setAttribute(PDO::ATTR_EMULATE_PREPARES, true);
        $statement = $db->prepare($query);
        $statement->execute();
        // disable emulation again. not an issue if this line is not reached because of an exception
        $db->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
        return true;
    } catch (PDOException $e) {
        echo '<pre>';
        print_r($e);
        echo '</pre>';
        return false;
    } catch (Exception $e) {
        return false;
    }
}

function get_row_id_on_equals($table, $column, $value) {
    $result = execute_sql("select id from $table where $column = ?", [$value], true);
    if (!$result) {
        return 0;
    }
    return intval($result['id']);
}

// id = 0  auto increment used if enabled. 0 if not
// id = -1 no id is added
// id = ?  any other value will be used as is
function insert_into_table($table, $parameters, $id = 0) {
    $questions = make_question_marks(count($parameters));
    if ($id !== -1) {
        $id = intval($id);
        if (strlen($questions) > 0) {
            $questions = ',' . $questions;
        }
        return execute_sql("insert into $table values ($id $questions)", $parameters);
    }
    return execute_sql("insert into $table values ($questions)", $parameters);
}
