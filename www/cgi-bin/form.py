#!/usr/bin/env python3
"""
Robust CGI Form Handler
- Safe HTML escaping
- GET and POST unified parsing
- Handles urlencoded and multipart/form-data
- Size limits
"""

import os
import sys
import html
import cgi
import cgitb

# Enable CGI error reporting (development only)
cgitb.enable()

MAX_POST_SIZE = 1024 * 1024  # 1MB limit


# ------------------------
# Helpers
# ------------------------

def h(text):
    """HTML escape"""
    return html.escape(str(text), quote=True)


def print_headers():
    print("Content-Type: text/html; charset=utf-8")
    print("")


def page_top():
    print("""<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Form Handler</title>
<style>
body { font-family: Arial, sans-serif; background:#f5f5f5; }
.container { background:white; max-width:700px; margin:40px auto;
padding:30px; border-radius:8px; }
table { width:100%; border-collapse:collapse; }
th,td { padding:8px; border-bottom:1px solid #ddd; }
th { background:#f0f0f0; }
.data { background:#e8f4e8; padding:15px; border-radius:5px; }
.error { background:#f4e8e8; padding:15px; border-radius:5px; }
</style>
</head>
<body>
<div class="container">
<h1>Form Data Received</h1>
""")


def page_bottom():
    print("""
<h2>Test Form</h2>

<form method="POST" action="/cgi-bin/form.py">
<p>Name: <input name="name" value="Test User"></p>
<p>Email: <input name="email" value="test@example.com"></p>
<p>Message:<br>
<textarea name="message">Hello CGI</textarea></p>
<button type="submit">Submit</button>
</form>

<p><a href="/cgi-bin/form.py?x=42&y=hello">Test GET</a></p>
<p><a href="/cgi-bin/">← Back To Tests</a></p>
<p><a href="/">Back Home</a></p>
</div>
</body>
</html>
""")


def show_table(fields):
    print("<div class='data'><table>")
    print("<tr><th>Field</th><th>Value</th></tr>")
    for key in fields:
        values = fields.getlist(key)
        for v in values:
            print(f"<tr><td>{h(key)}</td><td>{h(v)}</td></tr>")
    print("</table></div>")


# ------------------------
# Main
# ------------------------

print_headers()
page_top()

method = os.environ.get("REQUEST_METHOD", "GET")
print(f"<p><b>Request Method:</b> {h(method)}</p>")

# Reject oversized bodies
cl = os.environ.get("CONTENT_LENGTH")
if cl and int(cl) > MAX_POST_SIZE:
    print("<div class='error'>POST body too large</div>")
    page_bottom()
    sys.exit(0)

# cgi.FieldStorage handles GET + POST automatically
form = cgi.FieldStorage()

if len(form) == 0:
    print("<div class='error'>No parameters received</div>")
else:
    show_table(form)


page_bottom()