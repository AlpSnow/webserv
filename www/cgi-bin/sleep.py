#!/usr/bin/python3
import time

# Freeze the script for 3 seconds
time.sleep(3)

print("Content-Type: text/plain\r\n\r\n", end="")
print("Python finally woke up!")