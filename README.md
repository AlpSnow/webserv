*This project has been created as part of the 42 curriculum by mlavry, cnamoune, and mwallis.*

# Webserv

## Description

Webserv is a custom HTTP server written in C++98. The goal of the project is to reproduce the core behaviour of a real web server: opening listening sockets, accepting browser connections, parsing HTTP requests, routing them through a configuration file, generating HTTP responses, serving files, handling uploads, deleting resources, and executing CGI scripts.

The project is inspired by Nginx-style configuration files and focuses on low-level networking rather than using a framework or an existing HTTP server. It uses a single `poll()`-based event loop to monitor listening sockets, client sockets, and CGI pipes, allowing the server to handle several clients without one process or one thread per connection.

## Main Features

- HTTP server written in C++98
- Configuration file passed as an argument, with `configs/default.conf` used by default
- Multiple `server` blocks
- Multiple listening ports and host:port pairs
- Server-name based selection for requests received on the same listening socket
- Non-blocking listening sockets and client sockets
- Single `poll()` loop for network I/O and CGI pipe I/O
- HTTP request parsing with support for headers, bodies, query strings, `Content-Length`, and chunked transfer bodies
- HTTP/1.1 request validation
- Supported methods: `GET`, `POST`, `DELETE`, and `HEAD`
- Static file serving
- Directory handling with index files and autoindex
- File upload support
- CGI execution for configured file extensions
- Custom error pages when configured
- Default generated error pages when no custom page is available
- HTTP redirections through the configuration file
- Keep-alive support unless the client asks for `Connection: close`
- Client and CGI timeouts to avoid indefinite hangs

## Project Structure

```text
webserv/
├── Makefile
├── main.cpp
├── config_loader/
│   ├── Config.hpp
│   ├── Config.cpp
│   ├── ConfigParser.hpp
│   └── ConfigParser.cpp
├── srcs/
│   ├── Server.hpp / Server.cpp
│   ├── Client.hpp / Client.cpp
│   ├── Request.hpp / Request.cpp
│   ├── Response.hpp / Response.cpp
│   └── CgiHandler.hpp / CgiHandler.cpp
├── configs/
│   ├── default.conf
│   └── invalid.conf
├── www/
│   ├── index.html
│   ├── cgi-bin/
│   ├── error_pages/
│   ├── directory1/
│   ├── directory2/
│   └── images/
├── phppendu/
│   └── PHP hangman website used to test PHP CGI
└── YoupiBanane/
    └── test files used for the 42 tester configuration
```

## Instructions

### Requirements

The project must be built with a C++ compiler supporting C++98.

Useful tools for testing:

- `curl`
- a standard web browser
- `telnet` or `nc`
- `python3`
- `php-cgi` if PHP CGI routes are tested

### Compilation

Compile the server:

```sh
make
```

Remove object files:

```sh
make clean
```

Remove object files and the executable:

```sh
make fclean
```

Rebuild everything:

```sh
make re
```

The executable produced by the Makefile is:

```sh
./webserv
```

### Execution

Run the server with the default configuration:

```sh
./webserv
```

This loads:

```text
configs/default.conf
```

Run the server with a custom configuration file:

```sh
./webserv configs/<file.conf>
```

The argument must be a `.conf` file. If too many arguments are provided, or if the file extension is not `.conf`, the program exits with an error message.

### Default Configuration

The default configuration defines several servers:

- `0.0.0.0:8080` for the main static website and CGI examples
- `0.0.0.0:8085` for the PHP hangman website in `phppendu/`
- `127.0.0.1:8082` for the 42 tester-oriented `YoupiBanane` configuration

Example URLs:

```text
http://localhost:8080/
http://localhost:8080/images/
http://localhost:8080/old-page
http://localhost:8080/cgi-bin/index.py
http://localhost:8085/
http://127.0.0.1:8082/
```

If one of these ports is already used on your machine, edit `configs/default.conf` and replace the corresponding `listen` value.

## Configuration File

The configuration syntax is inspired by the `server` blocks of Nginx.

Example:

```nginx
server {
    listen 8080;
    server_name localhost webserv.local;

    root www;
    index index.html;
    client_max_body_size 1000000;

    error_page 404 error_pages/404.html;

    location /images {
        methods GET HEAD POST DELETE;
        upload on;
        upload_store www/images;
        autoindex on;
    }

    location /old-page {
        methods GET HEAD;
        return 301 /images;
    }

    location /cgi-bin {
        methods GET POST HEAD;
        root www/cgi-bin;
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php-cgi;
        autoindex off;
    }
}
```

Supported server-level directives:

- `listen`
- `server_name`
- `root`
- `index`
- `client_max_body_size`
- `error_page`
- `location`

Supported location-level directives:

- `methods`
- `root`
- `index`
- `autoindex`
- `return`
- `client_max_body_size`
- `upload`
- `upload_store`
- `cgi`

The parser validates malformed configuration files and reports errors such as duplicate directives, invalid ports, invalid methods, invalid CGI declarations, missing semicolons, unknown directives, invalid location paths, and invalid upload configuration.

## HTTP Behaviour

### Request Parsing

The request parser reads data progressively from the client socket. It parses:

- request line
- HTTP method
- path and query string
- HTTP version
- headers
- body with `Content-Length`
- body with `Transfer-Encoding: chunked`

A request is presented as follow for the server ;

GET / HTTP/1.1\r\n
Host: google.com\r\n
Accept: text/html\r\n
User-Agent: NCAT/1.0\r\n
Connection: keep-alive\r\n\r\n

Unsupported HTTP versions return `505 HTTP Version Not Supported`. Unsupported methods outside the implemented set return `501 Not Implemented`. Malformed requests return `400 Bad Request`.

### Routing

For each request, the server selects the active `server` block from the listening socket and the `Host` header. It then selects the best matching `location` by longest URL-prefix match.

The selected location controls the allowed methods, root directory, index file, CGI handlers, upload settings, autoindex behaviour, and redirections.

### Responses

The response builder generates valid HTTP/1.1 responses with status line, headers, and body. It supports common status codes such as:

- `200 OK`
- `201 Created`
- `204 No Content`
- `301 Moved Permanently`
- `400 Bad Request`
- `403 Forbidden`
- `404 Not Found`
- `405 Method Not Allowed`
- `408 Request Timeout`
- `413 Payload Too Large`
- `500 Internal Server Error`
- `501 Not Implemented`
- `504 Gateway Timeout`
- `505 HTTP Version Not Supported`

Custom error pages are loaded from the configured `error_page` directives when available. Otherwise, the server generates a simple default HTML error page.

## CGI

CGI is configured per location and per file extension.

Example:

```nginx
location /cgi-bin {
    methods GET POST HEAD;
    root www/cgi-bin;
    cgi .py /usr/bin/python3;
    cgi .php /usr/bin/php-cgi;
    cgi .sh /bin/bash;
}
```

When a request targets a file whose extension matches a configured CGI handler, the server executes the configured interpreter with `execve()`.

The CGI handler prepares environment variables such as:

- `REQUEST_METHOD`
- `QUERY_STRING`
- `PATH_INFO`
- `SCRIPT_FILENAME`
- `SERVER_PROTOCOL`
- `REDIRECT_STATUS`
- `CONTENT_LENGTH` for POST requests
- `CONTENT_TYPE` when provided
- HTTP headers converted to CGI-style `HTTP_*` variables

For POST CGI requests, the request body is written to the CGI process through a non-blocking pipe. The CGI output is read through another pipe, parsed into headers and body, and then converted into an HTTP response.

[C++ SERVER]                                          [PYTHON SCRIPT]

Server writes POST        ====== fd_in ======        Python reads POST
body into fd_in[1]  --->  [1]           [0]  --->    from STDIN (0)
(The Write End)                                      (The Read End)

Server reads HTML         ====== fd_out =====        Python writes HTML
from fd_out[0]      <---  [0]           [1]  <---    into STDOUT (1)
(The Read End)                                       (The Write End)

A CGI timeout is implemented to prevent a CGI process from blocking the server indefinitely.

## Uploads

Uploads are enabled per location:

```nginx
location /images {
    methods GET HEAD POST DELETE;
    upload on;
    upload_store www/images;
    autoindex on;
}
```

The server supports multipart form uploads and stores uploaded files in the configured `upload_store` directory. If a request body is larger than the active `client_max_body_size`, the server returns `413 Payload Too Large`.

## Testing Examples

Static file:

```sh
curl -i http://localhost:8080/
```

Autoindex:

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

POST body:

```sh
curl -i -X POST http://localhost:8080/ -d "hello webserv"
```

Upload a file:

```sh
curl -i -F "file=@README.md" http://localhost:8080/images/
```

Delete an uploaded file:

```sh
curl -i -X DELETE http://localhost:8080/images/README.md
```

PHP hangman website:

```text
http://localhost:8085/
```

## Resources

Classic references used or useful for this project:

- RFC 9110: HTTP Semantics
- RFC 9112: HTTP/1.1
- RFC 3875: CGI/1.1
- Nginx documentation, especially `server`, `location`, `root`, `index`, `error_page`, and `return`
- Linux manual pages for `socket`, `bind`, `listen`, `accept`, `poll`, `recv`, `send`, `fcntl`, `pipe`, `fork`, `execve`, `dup2`, `waitpid`, `stat`, `opendir`, and `readdir`
- C++ reference documentation for C++98 containers, streams, strings, maps, sets, and vectors
- Browser, `curl`, `telnet`, and Python-based tests for comparing server behaviour

### AI Usage

AI tools were used as assistance for repetitive and documentation-oriented tasks. In particular, AI was used to:

- improve the clarity and structure of documentation
- summarize implemented features from the source code
- propose test ideas and `curl` examples
- help compare expected HTTP behaviour with the subject requirements
- review wording for the README and auxiliary documentation

AI-generated suggestions were reviewed manually against the project source code, the configuration files, and the Webserv subject before being kept. The implementation remains the responsibility of the project team.
