#!/usr/bin/php-cgi
<?php
// Set the timezone to match your local time
date_default_timezone_set('Europe/Paris');

// Get the current date and time
$current_time = date('l, j F Y - H:i:s');
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Server Time</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
            background-color: #f4f4f9;
            color: #333;
        }
        .time-box {
            background: white;
            display: inline-block;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 4px 8px rgba(0,0,0,0.1);
        }
        h1 { color: #0066cc; }
        .time {
            font-size: 32px;
            font-weight: bold;
            color: #ff4d4d;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="time-box">
        <h1>⏰ Server Time</h1>
        <p>This page was generated dynamically by PHP via CGI.</p>
        <div class="time"><?php echo $current_time; ?></div>
        <br>
        <a href="/" style="color: #0066cc; text-decoration: none; font-weight: bold;">&larr; Back to Test Hub</a>
    </div>
</body>
</html>