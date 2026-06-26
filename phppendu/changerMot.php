<?php 
    session_start();

    unset($_SESSION['mot']);

    header("location:index.php");
    exit();
?>