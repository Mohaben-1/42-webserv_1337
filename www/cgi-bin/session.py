#!/usr/bin/env python3
"""
Simple Session Demo
"""
import os
import json
import time
import hashlib
from urllib.parse import parse_qs

SESSION_FILE = "/tmp/webserv_sessions.json"

def load_sessions():
    try:
        with open(SESSION_FILE, 'r') as f:
            return json.load(f)
    except:
        return {}

def save_sessions(sessions):
    with open(SESSION_FILE, 'w') as f:
        json.dump(sessions, f)

def get_cookie(name):
    for item in os.environ.get('HTTP_COOKIE', '').split(';'):
        if '=' in item:
            k, v = item.strip().split('=', 1)
            if k == name:
                return v
    return ''

def generate_session_id():
    return hashlib.sha256(str(time.time()).encode()).hexdigest()[:32]

# Parse request
query = parse_qs(os.environ.get('QUERY_STRING', ''))
action = query.get('action', ['view'])[0]
username = query.get('username', [''])[0]

# Get session
session_id = get_cookie('session_id')
sessions = load_sessions()
session = sessions.get(session_id, {})

# Start HTML
def html_head(title):
    return f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>{title}</title>
    <style>
        * {{ margin: 0; padding: 0; box-sizing: border-box; }}
        body {{ font-family: -apple-system, sans-serif; background: #1a1a2e; color: #eee; min-height: 100vh; padding: 40px 20px; }}
        .container {{ max-width: 500px; margin: 0 auto; }}
        h1 {{ text-align: center; margin-bottom: 30px; color: #667eea; }}
        .card {{ background: #16213e; border-radius: 12px; padding: 20px; margin-bottom: 20px; }}
        .card h2 {{ font-size: 14px; color: #888; margin-bottom: 15px; text-transform: uppercase; }}
        .info-row {{ display: flex; justify-content: space-between; padding: 12px; background: #0f3460; border-radius: 6px; margin-bottom: 8px; }}
        .label {{ color: #888; }}
        .value {{ color: #4ecca3; font-family: monospace; }}
        .success {{ background: #4ecca3; color: #1a1a2e; padding: 15px; border-radius: 8px; margin-bottom: 20px; text-align: center; }}
        .links {{ list-style: none; }}
        .links li {{ margin-bottom: 10px; }}
        .links a {{ color: #667eea; text-decoration: none; padding: 12px 15px; display: block; background: #0f3460; border-radius: 6px; transition: 0.2s; text-align: center; }}
        .links a:hover {{ background: #667eea; color: white; }}
        .links a.danger {{ background: #e94560; color: white; }}
        .links a.danger:hover {{ background: #ff6b6b; }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🔐 Session Demo</h1>"""

html_foot = """    </div>
</body>
</html>"""

# Handle login
if action == 'login' and username:
    session_id = generate_session_id()
    sessions[session_id] = {'username': username, 'visits': 1, 'created': time.time()}
    save_sessions(sessions)
    
    print("Content-Type: text/html")
    print(f"Set-Cookie: session_id={session_id}; Path=/; HttpOnly")
    print()
    print(html_head("Logged In"))
    print(f"""        <div class="success">✓ Welcome, {username}!</div>
        <div class="card">
            <h2>Session Created</h2>
            <div class="info-row"><span class="label">Session ID</span><span class="value">{session_id[:16]}...</span></div>
        </div>
        <div class="card">
            <ul class="links">
                <li><a href="/cgi-bin/session.py">View Session</a></li>
            </ul>
        </div>""")
    print(html_foot)

# Handle logout
elif action == 'logout':
    if session_id in sessions:
        del sessions[session_id]
        save_sessions(sessions)
    
    print("Content-Type: text/html")
    print("Set-Cookie: session_id=; Max-Age=0; Path=/")
    print()
    print(html_head("Logged Out"))
    print("""        <div class="success">👋 You have been logged out</div>
        <div class="card">
            <ul class="links">
                <li><a href="/cgi-bin/session.py">Back to Login</a></li>
            </ul>
        </div>""")
    print(html_foot)

# View session
else:
    print("Content-Type: text/html")
    print()
    
    if session:
        session['visits'] = session.get('visits', 0) + 1
        sessions[session_id] = session
        save_sessions(sessions)
        
        print(html_head("Session Active"))
        print(f"""        <div class="success">✓ Logged in as {session['username']}</div>
        <div class="card">
            <h2>Session Info</h2>
            <div class="info-row"><span class="label">Username</span><span class="value">{session['username']}</span></div>
            <div class="info-row"><span class="label">Visit Count</span><span class="value">{session['visits']}</span></div>
            <div class="info-row"><span class="label">Session ID</span><span class="value">{session_id[:16]}...</span></div>
        </div>
        <div class="card">
            <ul class="links">
                <li><a href="/cgi-bin/session.py">🔄 Refresh (increment visits)</a></li>
                <li><a href="?action=logout" class="danger">🚪 Logout</a></li>
            </ul>
        </div>""")
        print(html_foot)
    else:
        print(html_head("Session Demo"))
        print("""        <div class="card">
            <h2>You are not logged in</h2>
            <p style="color: #888; margin-bottom: 15px;">Click below to start a session:</p>
            <ul class="links">
                <li><a href="?action=login&username=John">Login as John</a></li>
                <li><a href="?action=login&username=Alice">Login as Alice</a></li>
                <li><a href="?action=login&username=Bob">Login as Bob</a></li>
            </ul>
        </div>
        <div class="card">
            <ul class="links">
                <li><a href="/cgi-bin/">← Back To Tests</a></li>
                <li><a href="/">← Back Home</a></li>
            </ul>
        </div>""")
        print(html_foot)
