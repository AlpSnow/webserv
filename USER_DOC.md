# User Documentation

This document explains how to build, launch, use, and test the Webserv project as an end user or evaluator.

## What Webserv Does

Webserv is a custom HTTP server written in C++98. It can serve static files, execute CGI scripts, receive uploaded files, delete files when allowed by the configuration, generate directory listings, return custom error pages, and run multiple configured websites on different ports.

## Build the Project

From the repository root, run:

```sh
make
```

This creates the executable:

```sh
./webserv
```

Useful Makefile commands:

```sh
make clean    # remove object files
make fclean   # remove object files and the webserv executable
make re       # rebuild from scratch
```

## Start the Server

Start with the default configuration:

```sh
./webserv
```

This uses:

```text
configs/default.conf
```

Start with a custom configuration file:

```sh
./webserv path/to/config.conf
```

The configuration file must use the `.conf` extension.

## Stop the Server

Press:

```text
Ctrl + C
```

The program catches `SIGINT` and stops the main loop before closing its file descriptors.

You can also stop it from another terminal with:

```sh
pkill webserv
```

or, if you know the process id:

```sh
kill <pid>
```

## Restart the Server

If the executable already exists:

```sh
make re
./webserv
```

If you only changed the configuration file, recompilation is not required:

```sh
./webserv configs/default.conf
```

## Default Websites

The default configuration exposes three main listening entries.

### Main website

```text
http://localhost:8080/
```

Useful routes:

```text
http://localhost:8080/
http://localhost:8080/images/
http://localhost:8080/old-page
http://localhost:8080/cgi-bin/index.py
```

The `/images` route has upload and autoindex enabled in the default configuration.

### PHP hangman website

```text
http://localhost:8085/
```

This route serves the `phppendu/` website. It requires a working `php-cgi` executable for PHP execution.

### 42 tester-oriented configuration

```text
http://127.0.0.1:8082/
```

This part of the default configuration is intended for tests around the `YoupiBanane/` files and CGI tester.

## If a Port Is Already Used

If startup fails with an error similar to:

```text
Address already in use
```

open `configs/default.conf` and change the conflicting `listen` port. For example:

```nginx
listen 8080;
```

can be changed to:

```nginx
listen 8081;
```

Then restart the server:

```sh
./webserv configs/default.conf
```

## Browser Tests

Open a browser and visit:

```text
http://localhost:8080/
```

Try the following manually:

- navigate to `/images/` to see directory listing
- upload a file from the generated upload form if available
- visit `/old-page` to test redirection
- visit `/cgi-bin/index.py` to test Python CGI
- visit `http://localhost:8085/` to test the PHP hangman website

## Curl Tests

Static page:

```sh
curl -i http://localhost:8080/
```

Directory listing:

```sh
curl -i http://localhost:8080/images/
```

Redirection:

```sh
curl -i http://localhost:8080/old-page
```

Python CGI:

```sh
curl -i http://localhost:8080/cgi-bin/index.py
```

POST request:

```sh
curl -i -X POST http://localhost:8080/ -d "hello webserv"
```

Upload a file:

```sh
curl -i -F "file=@README.md" http://localhost:8080/images/
```

Delete the uploaded file:

```sh
curl -i -X DELETE http://localhost:8080/images/README.md
```

Test the 42 tester port:

```sh
curl -i http://127.0.0.1:8082/
```

## Configuration Overview

A configuration file is made of one or more `server` blocks:

```nginx
server {
    listen 8080;
    server_name localhost;
    root www;
    index index.html;

    location / {
        methods GET POST;
        autoindex on;
    }
}
```

Common directives:

- `listen`: host and port where the server listens
- `server_name`: names accepted for the server block
- `root`: directory used to find files
- `index`: default file served for a directory
- `client_max_body_size`: maximum accepted request body size
- `error_page`: custom error page path
- `location`: route-specific configuration
- `methods`: allowed HTTP methods for a route
- `autoindex`: directory listing on or off
- `return`: HTTP redirection
- `upload`: enables or disables uploads
- `upload_store`: directory where uploaded files are stored
- `cgi`: maps a file extension to a CGI executable

## CGI Requirements

The default configuration may use:

```text
/usr/bin/python3
/usr/bin/php-cgi
/bin/bash
```

Check that the required executables exist before testing those CGI routes:

```sh
which python3
which php-cgi
which bash
```

If an executable is not installed or is located elsewhere, update the matching `cgi` directive in the configuration file.

## Expected Shutdown Behaviour

During normal shutdown, the server closes open file descriptors and exits. During a timeout, the server can return `408 Request Timeout` for clients or `504 Gateway Timeout` for CGI execution that takes too long.
