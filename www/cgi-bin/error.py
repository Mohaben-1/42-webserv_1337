#!/usr/bin/env python3
"""
CGI script for testing error handling
Usage: /cgi-bin/error.py?type=exception|exit|timeout|divide
"""
import os
import sys
import urllib.parse

query_string = os.environ.get('QUERY_STRING', '')
params = dict(urllib.parse.parse_qsl(query_string))
error_type = params.get('type', '')

if error_type == 'exit':
    print("Content-Type: text/html")
    print("")
    print("<h1>Exiting with code 1...</h1>")
    sys.stdout.flush()
    sys.exit(1)

elif error_type == 'exception':
    print("Content-Type: text/html")
    print("")
    raise Exception("Test exception raised!")

elif error_type == 'no_output':
    sys.exit(0)

elif error_type == 'divide':
    print("Content-Type: text/html")
    print("")
    x = 1 / 0

elif error_type == 'timeout':
    import time
    print("Content-Type: text/html")
    print("")
    print("<h1>Sleeping forever...</h1>")
    sys.stdout.flush()
    while True:
        time.sleep(10)

else:
    print("Content-Type: text/html")
    print("")
    print("""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Error Test</title>
    <style>
        body { font-family: sans-serif; background: #1a1a2e; color: #fff; padding: 40px; }
        .card { background: #16213e; padding: 30px; border-radius: 12px; max-width: 500px; }
        h1 { color: #e94560; margin-top: 0; }
        a { display: block; padding: 12px; margin: 8px 0; background: #0f3460; color: #fff; 
            text-decoration: none; border-radius: 6px; transition: background 0.2s; }
        a:hover { background: #e94560; }
        .warn { color: #f59e0b; font-size: 12px; }
    </style>
</head>
<body>
    <div class="card">
        <h1>⚠️ Error Test</h1>
        <p>Test how the server handles CGI errors:</p>
        <a href="?type=exception">🔥 Raise Exception</a>
        <a href="?type=exit">🚪 Exit Code 1</a>
        <a href="?type=divide">➗ Division by Zero</a>
        <a href="?type=no_output">🔇 No Output</a>
        <a href="?type=timeout">⏱️ Infinite Loop</a>
        <p class="warn">⚠️ These will cause errors - for testing only!</p>
    </div>
</body>
</html>""")
