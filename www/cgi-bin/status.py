#!/usr/bin/env python3
"""
CGI script demonstrating custom HTTP status codes
Usage: /cgi-bin/status.py?code=404
"""
import os
import urllib.parse

query_string = os.environ.get('QUERY_STRING', '')
params = dict(urllib.parse.parse_qsl(query_string))
code = params.get('code', '200')

status_map = {
    '200': ('OK', '#10b981', 'Success'),
    '201': ('Created', '#10b981', 'Resource created'),
    '204': ('No Content', '#6b7280', 'Empty response'),
    '301': ('Moved Permanently', '#3b82f6', 'Permanent redirect'),
    '302': ('Found', '#3b82f6', 'Temporary redirect'),
    '400': ('Bad Request', '#f59e0b', 'Invalid request'),
    '401': ('Unauthorized', '#f59e0b', 'Auth required'),
    '403': ('Forbidden', '#ef4444', 'Access denied'),
    '404': ('Not Found', '#ef4444', 'Resource missing'),
    '500': ('Internal Server Error', '#ef4444', 'Server error'),
    '502': ('Bad Gateway', '#ef4444', 'Gateway error'),
    '503': ('Service Unavailable', '#ef4444', 'Service down'),
}

msg, color, desc = status_map.get(code, ('Unknown', '#6b7280', 'Unknown status'))

print(f"Status: {code} {msg}")
print("Content-Type: text/html")
print("")

print(f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Status {code}</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{ 
            font-family: -apple-system, sans-serif; 
            background: #0f172a; 
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #fff;
        }}
        .card {{
            background: #1e293b;
            padding: 60px;
            border-radius: 20px;
            text-align: center;
            box-shadow: 0 25px 50px rgba(0,0,0,0.5);
        }}
        .code {{
            font-size: 120px;
            font-weight: 800;
            color: {color};
            line-height: 1;
        }}
        .msg {{ font-size: 28px; margin: 15px 0 5px; }}
        .desc {{ color: #64748b; margin-bottom: 40px; }}
        .links {{ display: flex; gap: 10px; flex-wrap: wrap; justify-content: center; }}
        .links a {{
            padding: 8px 16px;
            background: #334155;
            color: #94a3b8;
            text-decoration: none;
            border-radius: 6px;
            font-size: 14px;
            transition: all 0.2s;
        }}
        .links a:hover {{ background: #475569; color: #fff; }}
        .back {{ margin-top: 30px; }}
        .back a {{ color: #3b82f6; text-decoration: none; }}
    </style>
</head>
<body>
    <div class="card">
        <div class="code">{code}</div>
        <div class="msg">{msg}</div>
        <div class="desc">{desc}</div>
        <div class="links">
            <a href="?code=200">200</a>
            <a href="?code=201">201</a>
            <a href="?code=400">400</a>
            <a href="?code=401">401</a>
            <a href="?code=403">403</a>
            <a href="?code=404">404</a>
            <a href="?code=500">500</a>
            <a href="?code=503">503</a>
        </div>
        <div class="back"><a href="/cgi-bin/">← Back To Tests</a></div>
        <div class="back"><a href="/">← Back Home</a></div>
    </div>
</body>
</html>""")