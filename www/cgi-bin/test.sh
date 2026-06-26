#!/bin/bash

# 1. Print the mandatory HTTP headers and the blank line
# (Using printf is safer in Bash to guarantee the \r\n characters)
printf "Content-Type: text/html\r\n\r\n"

# 2. Print the HTML body
printf "<html>"
printf "<head><title>Bash CGI</title></head>"
printf "<body style=\"font-family: Arial; text-align: center; margin-top: 50px;\">"
printf "  <h1>Hello from Bash!</h1>"
printf "  <p>Bash scripts are super fast for simple CGI tasks.</p>"

# Test the query string (e.g., http://localhost:8080/cgi-bin/test.sh?name=webserv)
printf "  <p><strong>Query String:</strong> ${QUERY_STRING:-"Variable Missing!"}</p>"

printf "</body>"
printf "</html>"