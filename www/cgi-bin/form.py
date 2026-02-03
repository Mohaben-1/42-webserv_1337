#!/usr/bin/env python3
"""
CGI script to handle form data (POST)
"""
import os
import sys
import urllib.parse

# Output headers
print("Content-Type: text/html")
print("")

print("""<!DOCTYPE html>
<html>
<head>
    <title>Form Handler</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; max-width: 600px; margin: 0 auto; }
        h1 { color: #333; }
        .data { background: #e8f4e8; padding: 15px; border-radius: 4px; margin: 10px 0; }
        .error { background: #f4e8e8; padding: 15px; border-radius: 4px; margin: 10px 0; }
        table { width: 100%; border-collapse: collapse; }
        td, th { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background: #f0f0f0; }
        a { color: #0066cc; }
    </style>
</head>
<body>
<div class="container">
    <h1>Form Data Received</h1>
""")

method = os.environ.get('REQUEST_METHOD', 'GET')
print(f"<p><strong>Request Method:</strong> {method}</p>")

if method == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    content_type = os.environ.get('CONTENT_TYPE', '')
    
    print(f"<p><strong>Content-Type:</strong> {content_type}</p>")
    print(f"<p><strong>Content-Length:</strong> {content_length}</p>")
    
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print(f"<p><strong>Raw Data:</strong></p>")
        print(f"<pre>{post_data[:500]}{'...' if len(post_data) > 500 else ''}</pre>")
        
        # Parse form data
        if 'application/x-www-form-urlencoded' in content_type:
            print("<div class='data'><h3>Parsed Form Fields:</h3><table>")
            print("<tr><th>Field</th><th>Value</th></tr>")
            params = urllib.parse.parse_qs(post_data)
            for key, values in params.items():
                for value in values:
                    print(f"<tr><td>{key}</td><td>{value}</td></tr>")
            print("</table></div>")
    else:
        print("<div class='error'>No POST data received</div>")

elif method == 'GET':
    query_string = os.environ.get('QUERY_STRING', '')
    print(f"<p><strong>Query String:</strong> {query_string or '(empty)'}</p>")
    
    if query_string:
        print("<div class='data'><h3>Query Parameters:</h3><table>")
        print("<tr><th>Parameter</th><th>Value</th></tr>")
        params = urllib.parse.parse_qs(query_string)
        for key, values in params.items():
            for value in values:
                print(f"<tr><td>{key}</td><td>{value}</td></tr>")
        print("</table></div>")

print("""
    <h2>Test Form</h2>
    <form method="POST" action="/cgi-bin/form.py">
        <p>
            <label>Name: <input type="text" name="name" value="Test User"></label>
        </p>
        <p>
            <label>Email: <input type="email" name="email" value="test@example.com"></label>
        </p>
        <p>
            <label>Message:<br>
            <textarea name="message" rows="3" cols="40">Hello from CGI!</textarea>
            </label>
        </p>
        <p>
            <button type="submit">Submit POST</button>
        </p>
    </form>
    
    <p><a href="/cgi-bin/form.py?name=URLTest&value=123">Test GET with query string</a></p>
    <p><a href="/cgi-bin/test.py">Back to CGI Test</a></p>
</div>
</body>
</html>
""")
