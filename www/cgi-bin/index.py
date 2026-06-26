#!/usr/bin/python3
import os
import sys

# FIX: Use sys.stdout.write to output exact HTTP delimiters
sys.stdout.write("Content-Type: text/html\r\n\r\n")

print("<html>")
print("<head><title>Python CGI</title></head>")
print("<body style=\"font-family: Arial; text-align: center; margin-top: 50px;\">")
print("  <h1>Hello from Python!</h1>")
print("  <p>If you see this, your C++ execve() worked perfectly.</p>")

method = os.environ.get("REQUEST_METHOD", "Variable Missing!")
print("  <p><strong>HTTP Method Used:</strong> " + method + "</p>")

print("</body>")
print("</html>")