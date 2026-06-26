# Developer Documentation

This document explains the internal architecture of the Webserv project and how the main source files interact.

## Build and Run

Compile from the repository root:

```sh
make
```

Run with the default configuration:

```sh
./webserv
```

Run with a custom configuration:

```sh
./webserv configs/default.conf
```

Clean build artifacts:

```sh
make clean
make fclean
make re
```

The project is compiled with:

```text
-Wall -Wextra -Werror -std=c++98
```

## High-Level Architecture

The server is organized around five main components:

```text
Config / ConfigParser
        ↓
Server
        ↓
Client
        ↓
Request parser
        ↓
Response builder
        ↓
CGI handler, filesystem, upload, delete, autoindex, error pages
```

The program starts in `main.cpp`, loads a configuration file, creates a `Server` object, initializes listening sockets, and then enters the event loop.

## Startup Flow

1. `main.cpp` installs signal handlers for `SIGINT` and `SIGTERM`.
2. It selects the configuration path:
   - no argument: `configs/default.conf`
   - one argument: user-provided `.conf` file
3. It rejects non-`.conf` paths.
4. It constructs a `Config` object.
5. It gives the parsed `ServerConfig` list to `Server::setConfig()`.
6. It calls `Server::initServer()`.
7. It calls `Server::run()`.

## Configuration System

Files:

```text
config_loader/Config.hpp
config_loader/Config.cpp
config_loader/ConfigParser.hpp
config_loader/ConfigParser.cpp
```

The configuration loader stores parsed data in these main structures:

- `ListenConfig`
- `LocationConfig`
- `ServerConfig`

### ServerConfig

Stores server-level information:

- listening host and port pairs
- server names
- root directory
- index file
- maximum client body size
- custom error pages
- locations

### LocationConfig

Stores route-level information:

- URL path
- optional route root
- optional route index
- allowed methods
- autoindex flag
- route-specific maximum body size
- redirection code and target
- upload settings
- upload storage directory
- CGI extension-to-executable map

### Parser Responsibilities

The parser validates the configuration before the server starts. It rejects invalid syntax and invalid directive combinations, including:

- unknown directives
- missing semicolons
- duplicate directives where duplicates are not allowed
- invalid `listen` format
- ports outside `1..65535`
- invalid methods
- duplicate methods
- invalid `autoindex` values
- invalid redirection codes outside the `3xx` range
- `upload_store` without `upload on`
- `upload on` without `upload_store`
- CGI extensions that do not start with `.`
- duplicated CGI handlers for the same extension
- duplicated location paths in the same server
- server blocks without a `listen` directive

## Server Event Loop

Files:

```text
srcs/Server.hpp
srcs/Server.cpp
```

The `Server` class owns:

- `_fds`: vector of `pollfd` entries
- `_clients`: map from client fd to `Client`
- `_listenFdToServers`: mapping from listening fd to matching server blocks
- `_listenKeyToFd`: mapping from `host:port` to a listening fd
- `_configs`: parsed configuration list
- `client_pipe`: mapping from CGI pipe fd to owning client fd

### Socket Initialization

For each configured listen entry, `Server::initServer()`:

1. creates an IPv4 TCP socket with `socket(AF_INET, SOCK_STREAM, 0)`
2. enables `SO_REUSEADDR`
3. binds the socket to the configured host and port
4. calls `listen()`
5. sets the listening socket to non-blocking mode with `fcntl()`
6. adds the socket to `_fds` with `POLLIN`

If two server blocks share the same host and port, they share the same listening fd. The `Host` header is later used to select the correct server block when possible.

### Main Loop

`Server::run()` repeatedly calls:

```text
poll()
handleEvents()
checkTimeouts()
```

The loop stops when the global signal flag changes or when a fatal poll/server error occurs.

### Event Handling

`handleEvents()` distinguishes three kinds of file descriptors:

1. listening sockets
2. client sockets
3. CGI pipes

Listening sockets accept new clients when `POLLIN` is set.

Client sockets:

- `POLLIN`: read request data with `recv()` and feed the request parser
- `POLLOUT`: send response data with `send()`
- error or hangup events: close and remove the client when appropriate

CGI pipes:

- CGI stdin pipe uses `POLLOUT` to write the request body to the child process
- CGI stdout pipe uses `POLLIN` to read the script output

## Client State

Files:

```text
srcs/Client.hpp
srcs/Client.cpp
```

Each `Client` stores:

- client socket fd
- parsed request object
- request parser state machine
- response builder
- CGI handler
- current listen fd
- keep-alive status
- bytes sent
- timestamps for timeouts and logs
- response status code
- client IP string

After a complete keep-alive response, `resetClientForNextRequet()` resets parser and request state so the same socket can receive another request.

## Request Parser

Files:

```text
srcs/Request.hpp
srcs/Request.cpp
```

The request parser is implemented as a state machine:

```text
READING_REQUEST
READING_HEADER
READING_BODY
READING_CHUNKED
COMPLETE
ERROR
```

It parses data incrementally because a complete request may arrive in several `recv()` calls.

### Parsed Request Data

The `Request` structure stores:

- method
- path
- query string
- HTTP version
- headers
- keep-alive flag
- body as a `std::vector<char>`
- matched location pointer when needed

### Validation

The parser validates:

- request line format
- supported methods: `GET`, `POST`, `DELETE`, `HEAD`
- HTTP version: `HTTP/1.1`
- header format
- `Content-Length`
- `Transfer-Encoding: chunked`
- incompatibility between `Content-Length` and `Transfer-Encoding`

Error examples:

- malformed request line: `400`
- unsupported implemented method set: `501`
- unsupported HTTP version: `505`
- invalid body size: `413`

## Response Builder

Files:

```text
srcs/Response.hpp
srcs/Response.cpp
```

The `HttpResponse` class is responsible for turning a parsed request into raw bytes ready to send to the client.

Response states:

```text
RESPONSE_BUILDING
RESPONSE_READY_TO_SEND
RESPONSE_SENDING
RESPONSE_COMPLETE
```

### Routing

`translate_path()` chooses the best matching location using longest prefix matching. It then builds a final filesystem path from:

- server root or location root
- request path
- configured index file if the target is a directory

If a location has a redirection, response generation stops early and returns a `3xx` response with a `Location` header.

### Method Handlers

`HttpResponse::generate()` dispatches to:

- `handle_get()`
- `handle_post()`
- `handle_delete()`
- `handle_head()`
- `handle_cgi()` when a CGI extension matches

### GET

Serves a static file or builds an autoindex page for a directory if autoindex is enabled.

### POST

If `upload_store` is configured, writes the uploaded file to disk. Multipart form-data uploads are supported through filename extraction from the request body.

If no upload store is configured, the server returns a simple confirmation that the POST body was received.

### DELETE

Deletes the target file and returns `204 No Content` on success. Directories are rejected.

### HEAD

Builds headers for the target resource without appending the body to the final raw response.

### Error Responses

`generate_error()` first tries to load a custom configured error page. If none exists, it builds a default HTML error body.

## CGI Handler

Files:

```text
srcs/CgiHandler.hpp
srcs/CgiHandler.cpp
```

CGI states:

```text
STAND_BY
CGI_INIT
CGI_WRITING
CGI_READING
CGI_FINISHED
CGI_ERROR
```

### CGI Flow

1. `HttpResponse` detects a CGI extension configured for the matched location.
2. `CgiHandler::handle_script()` prepares CGI environment variables.
3. It creates two pipes:
   - one pipe for sending request body to CGI stdin
   - one pipe for reading CGI stdout
4. It forks.
5. The child process redirects stdin/stdout with `dup2()` and calls `execve()`.
6. The parent closes unused pipe ends and registers the remaining pipe fds in the main `poll()` loop.
7. The server writes the POST body if needed.
8. The server reads CGI output until EOF.
9. `HttpResponse::parse_cgi_output()` separates CGI headers from the CGI body and assembles the final HTTP response.

### CGI Environment Variables

The handler creates variables including:

- `REQUEST_METHOD`
- `QUERY_STRING`
- `PATH_INFO`
- `SCRIPT_FILENAME`
- `SERVER_PROTOCOL`
- `REDIRECT_STATUS`
- `CONTENT_LENGTH`
- `CONTENT_TYPE`
- converted HTTP headers as `HTTP_*`

### CGI Timeout

The server kills a CGI process that exceeds the configured timeout and generates a `504 Gateway Timeout` response.

## Timeouts

The server defines separate timeout values for:

- header reading
- body reading
- response sending
- CGI execution

Client timeouts can produce `408 Request Timeout`. CGI timeouts can produce `504 Gateway Timeout`.

## Logs

The server prints colored logs for requests and responses, including:

- timestamp
- client IP
- fd
- method
- path
- status code
- response size
- elapsed time

These logs are useful during manual browser, `curl`, and tester runs.

## Development Notes

### Adding a New Directive

To add a new configuration directive:

1. Add the storage field to `ServerConfig` or `LocationConfig`.
2. Add parsing logic in `ConfigParser.cpp`.
3. Add validation and duplicate checks.
4. Use the parsed value in `Server`, `Request`, or `Response` depending on the feature.
5. Add a valid example to `configs/default.conf`.
6. Add an invalid example or tester case if relevant.

### Adding a New HTTP Method

To add a method:

1. Accept it in `ClientRequest::parse_request_line()`.
2. Accept it in configuration parser method validation.
3. Add it to the location `methods` checks.
4. Implement a handler in `HttpResponse`.
5. Add tests with `curl -X METHOD`.

### Adding a New CGI Extension

Add a directive to the wanted location:

```nginx
cgi .ext /path/to/interpreter;
```

Then ensure:

- the interpreter exists
- the script file extension matches `.ext`
- the script prints valid CGI-style headers followed by an empty line

## Manual Testing Checklist

Build:

```sh
make re
```

Run:

```sh
./webserv configs/default.conf
```

Test:

```sh
curl -i http://localhost:8080/
curl -i http://localhost:8080/images/
curl -i http://localhost:8080/old-page
curl -i http://localhost:8080/cgi-bin/index.py
curl -i -X POST http://localhost:8080/ -d "hello"
curl -i -F "file=@README.md" http://localhost:8080/images/
curl -i -X DELETE http://localhost:8080/images/README.md
```

Invalid configuration test:

```sh
./webserv configs/invalid.conf
```

The server should reject invalid configuration files before opening the runtime server loop.
