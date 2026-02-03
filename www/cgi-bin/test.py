#!/usr/bin/env python3
# Simple CGI test script

import os
import sys

# CGI scripts must output headers first, then a blank line, then content
print("Content-Type: text/html")
print("")  # Blank line separates headers from body

print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Test</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; max-width: 800px; margin: 0 auto; }
        h1 { color: #2e7d32; }
        h2 { color: #1565c0; border-bottom: 2px solid #1565c0; padding-bottom: 10px; }
        ul { list-style: none; padding: 0; }
        li { background: #f8f8f8; margin: 5px 0; padding: 10px; border-radius: 4px; }
        li b { color: #333; min-width: 200px; display: inline-block; }
        .links { margin-top: 30px; }
        .links a { 
            display: inline-block; 
            margin: 5px; 
            padding: 10px 20px; 
            background: #1565c0; 
            color: white; 
            text-decoration: none; 
            border-radius: 4px; 
        }
        .links a:hover { background: #0d47a1; }
        .success { color: #2e7d32; }
    </style>
</head>
<body>
<div class="container">
    <h1>CGI Script Running!</h1>
    <p class="success">Python CGI is working correctly!</p>
    
    <h2>Environment Variables</h2>
    <ul>""")

# Print relevant CGI environment variables
cgi_vars = [
    'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
    'SCRIPT_NAME', 'SCRIPT_FILENAME', 'PATH_INFO', 'PATH_TRANSLATED',
    'SERVER_NAME', 'SERVER_PORT', 'SERVER_SOFTWARE', 'SERVER_PROTOCOL',
    'GATEWAY_INTERFACE', 'DOCUMENT_ROOT', 'REMOTE_ADDR',
    'HTTP_HOST', 'HTTP_USER_AGENT', 'HTTP_ACCEPT'
]

for var in cgi_vars:
    value = os.environ.get(var, '<not set>')
    print(f"<li><b>{var}:</b> {value}</li>")

print("""</ul>
    
    <div class="links">
        <h2>Test Other CGI Scripts</h2>
        <a href="/cgi-bin/form.py">Form Handler</a>
        <a href="/cgi-bin/api.py">JSON API</a>
        <a href="/cgi-bin/status.py">Status Codes</a>
        <a href="/cgi-bin/redirect.py?url=/cgi-bin/test.py">Redirect Test</a>
        <a href="/cgi-bin/test.py?name=test&value=123">uery String</a>
    </div>
    
    <p style="margin-top: 30px;"><a href="/">Back to Home</a></p>
</div>
</body>
</html>""")

