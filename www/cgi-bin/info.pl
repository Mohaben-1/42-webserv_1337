#!/usr/bin/env python3
"""
CGI script demonstrating HTTP redirects
Usage: /cgi-bin/redirect.py?url=/target or ?url=https://example.com
"""
import os
import urllib.parse

query_string = os.environ.get('QUERY_STRING', '')
params = dict(urllib.parse.parse_qsl(query_string))
target = params.get('url', '/')
code = params.get('code', '302')

status_map = {
    '301': '301 Moved Permanently',
    '302': '302 Found',
    '303': '303 See Other',
    '307': '307 Temporary Redirect',
    '308': '308 Permanent Redirect'
}
status = status_map.get(code, '302 Found')

print(f"Status: {status}")
print(f"Location: {target}")
print("Content-Type: text/html")
print("")
print(f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Redirecting...</title>
    <meta http-equiv="refresh" content="0;url={target}">
    <style>
        body {{ font-family: sans-serif; padding: 50px; text-align: center; background: #f0f0f0; }}
        a {{ color: #4f46e5; }}
    </style>
</head>
<body>
    <p>Redirecting to <a href="{target}">{target}</a>...</p>
</body>
</html>""")
