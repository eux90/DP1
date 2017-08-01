<?php
include ('php/functions.php');

$link = db_connect();
$query1 = 'TRUNCATE TABLE users';
$query2 = 'INSERT INTO users (username, password, thr, date) VALUES
("a@p.it", "f64551fcd6f07823cb87971cfb91446425da18286b3ab1ef935e0cbd7a69f68a", 0, "2017-06-09 11:34:30"),
("b@p.it", "3946ca64ff78d93ca61090a437cbb6b3d2ca0d488f5f9ccf3059608368b27693", 0, "2017-06-09 11:35:03"),
("c@p.it", "43bb00d0ce7790a53b91256b370c887b24791a5539a6fbfb70c5870e8c91ae5d", 0, "2017-06-09 11:36:29")';
$query3 = 'TRUNCATE TABLE bid';
$query4 = 'INSERT INTO bid (username, amount) VALUES
("nobody", 1)';

try{
  mysqli_autocommit($link, false);
  $result = mysqli_query($link, $query1);
  if (!$result) {
    throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
  }
  $result = mysqli_query($link, $query2);
  if (!$result) {
    throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
  }
  $result = mysqli_query($link, $query3);
  if (!$result) {
    throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
  }
  $result = mysqli_query($link, $query4);
  if (!$result) {
    throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
  }
  mysqli_commit($link);
  mysqli_autocommit($link, true);
  echo("RESET COMPLETE");
}
catch (Exception $e) {
  mysqli_rollback($link);
  echo $e->getMessage();
  mysqli_autocommit($link, true);
}

 ?>
