#!/usr/bin/env python3
"""
Simple Cookie Demo
"""
import os
from urllib.parse import parse_qs

def get_cookies():
    cookies = {}
    for item in os.environ.get('HTTP_COOKIE', '').split(';'):
        if '=' in item:
            key, value = item.strip().split('=', 1)
            cookies[key] = value
    return cookies

query = parse_qs(os.environ.get('QUERY_STRING', ''))
cookies = get_cookies()

# Start response
print("Content-Type: text/html")

# Handle clear
if 'clear' in query:
    for name in cookies:
        print(f"Set-Cookie: {name}=; Max-Age=0; Path=/")
    cookies = {}
    message = "All cookies cleared!"
# Handle set
elif query:
    for name, values in query.items():
        print(f"Set-Cookie: {name}={values[0]}; Path=/; Max-Age=3600")
        cookies[name] = values[0]
    message = "Cookie set!"
else:
    message = ""

print()
print("""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Cookie Demo</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, sans-serif; background: #1a1a2e; color: #eee; min-height: 100vh; padding: 40px 20px; }
        .container { max-width: 500px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 30px; color: #667eea; }
        .card { background: #16213e; border-radius: 12px; padding: 20px; margin-bottom: 20px; }
        .card h2 { font-size: 14px; color: #888; margin-bottom: 15px; text-transform: uppercase; }
        .cookie-item { display: flex; justify-content: space-between; padding: 10px; background: #0f3460; border-radius: 6px; margin-bottom: 8px; }
        .cookie-name { color: #667eea; font-weight: bold; }
        .cookie-value { color: #4ecca3; font-family: monospace; }
        .message { background: #4ecca3; color: #1a1a2e; padding: 12px; border-radius: 8px; margin-bottom: 20px; text-align: center; font-weight: bold; }
        .links { list-style: none; }
        .links li { margin-bottom: 10px; }
        .links a { color: #667eea; text-decoration: none; padding: 10px 15px; display: block; background: #0f3460; border-radius: 6px; transition: 0.2s; }
        .links a:hover { background: #667eea; color: white; }
        .empty { color: #666; text-align: center; padding: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🍪 Cookie Demo</h1>""")

if message:
    print(f'        <div class="message">{message}</div>')

print('        <div class="card">')
print('            <h2>Current Cookies</h2>')
if cookies:
    for name, value in cookies.items():
        print(f'            <div class="cookie-item"><span class="cookie-name">{name}</span><span class="cookie-value">{value}</span></div>')
else:
    print('            <div class="empty">No cookies set</div>')
print('        </div>')

print("""        <div class="card">
            <h2>Set a Cookie</h2>
            <ul class="links">
                <li><a href="?username=Medd">Set username = Medd</a></li>
                <li><a href="?theme=dark">Set theme = dark</a></li>
                <li><a href="?country=Morocco">Set country = Morocco</a></li>
            </ul>
        </div>
        <div class="card">
            <h2>Actions</h2>
            <ul class="links">
                <li><a href="?clear=1">🗑️ Clear All Cookies</a></li>
                <li><a href="/cgi-bin/">← Back To Tests</a></li>
                <li><a href="/">← Back Home</a></li>
            </ul>
        </div>
    </div>
</body>
</html>""")
