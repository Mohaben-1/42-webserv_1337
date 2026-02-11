#!/usr/bin/env python3
"""
Advanced Session Management Demo
A clean, organized CGI script with modern UI
"""
import os
import json
import time
import hashlib
import uuid
from urllib.parse import parse_qs, urlencode
from datetime import datetime
from typing import Dict, Any, Optional

# Configuration
SESSION_FILE = "/tmp/webserv_sessions.json"
COOKIE_NAME = 'session_id'

# Predefined users for demo
DEMO_USERS = [
    {"username": "Mohamed", "avatar": "👨‍💻", "role": "Developer", "color": "#2ea043"},
    {"username": "Aiman", "avatar": "👨‍💻", "role": "Developer", "color": "#2ea043"},
    {"username": "Jihad", "avatar": "👨‍💻", "role": "Developer", "color": "#2ea043"},
]

class SessionManager:
    """Handles all session operations"""
    
    def __init__(self):
        self.sessions = self._load()
    
    def _load(self) -> Dict:
        """Load sessions from file"""
        try:
            with open(SESSION_FILE, 'r') as f:
                return json.load(f)
        except (FileNotFoundError, json.JSONDecodeError):
            return {}
    
    def _save(self):
        """Save sessions to file"""
        with open(SESSION_FILE, 'w') as f:
            json.dump(self.sessions, f, indent=2)
    
    def create(self, username: str) -> str:
        """Create a new session"""
        session_id = hashlib.sha256(
            f"{username}{time.time()}{uuid.uuid4()}".encode()
        ).hexdigest()
        
        self.sessions[session_id] = {
            'username': username,
            'created': time.time(),
            'visits': 1,
            'user_agent': os.environ.get('HTTP_USER_AGENT', 'Unknown'),
            'ip': os.environ.get('REMOTE_ADDR', '127.0.0.1')
        }
        self._save()
        return session_id
    
    def get(self, session_id: str) -> Optional[Dict]:
        """Get session data if valid"""
        session = self.sessions.get(session_id)
        if session:
            session['visits'] = session.get('visits', 0) + 1
            self._save()
        return session
    
    def delete(self, session_id: str):
        """Delete a session"""
        if session_id in self.sessions:
            del self.sessions[session_id]
            self._save()

class HTMLRenderer:
    """Handles HTML rendering with consistent UI"""
    
    @staticmethod
    def header(title: str) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - Session Demo</title>
    <style>
        :root {{
            --bg-primary: #0a0c10;
            --bg-secondary: #161b22;
            --bg-card: #21262d;
            --accent: #2ea043;
            --accent-hover: #3fb950;
            --danger: #da3633;
            --danger-hover: #f85149;
            --text-primary: #f0f6fc;
            --text-secondary: #8b949e;
            --border: #30363d;
            --shadow: 0 8px 24px rgba(0,0,0,0.2);
        }}
        
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: var(--bg-primary);
            color: var(--text-primary);
            line-height: 1.6;
            min-height: 100vh;
            padding: 40px 20px;
        }}
        
        .container {{
            max-width: 800px;
            margin: 0 auto;
        }}
        
        .header {{
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 40px;
            padding-bottom: 20px;
            border-bottom: 1px solid var(--border);
        }}
        
        .logo {{
            display: flex;
            align-items: center;
            gap: 12px;
        }}
        
        .logo-icon {{
            font-size: 32px;
        }}
        
        .logo-text {{
            font-size: 24px;
            font-weight: 600;
            background: linear-gradient(45deg, #2ea043, #58a6ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }}
        
        .status-badge {{
            background: var(--bg-secondary);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .card {{
            background: var(--bg-secondary);
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 24px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow);
        }}
        
        .card-header {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid var(--border);
        }}
        
        .card-title {{
            font-size: 18px;
            font-weight: 600;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 20px;
        }}
        
        .stat-item {{
            background: var(--bg-card);
            padding: 16px;
            border-radius: 8px;
            border: 1px solid var(--border);
        }}
        
        .stat-label {{
            color: var(--text-secondary);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 4px;
        }}
        
        .stat-value {{
            font-size: 20px;
            font-weight: 600;
            color: var(--accent);
            font-family: monospace;
        }}
        
        .info-row {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px 16px;
            background: var(--bg-card);
            border-radius: 6px;
            margin-bottom: 8px;
            border: 1px solid var(--border);
        }}
        
        .info-label {{
            color: var(--text-secondary);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .info-value {{
            color: var(--text-primary);
            font-family: monospace;
            background: var(--bg-primary);
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 14px;
        }}
        
        .btn-group {{
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
        }}
        
        .btn {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            padding: 12px 24px;
            background: var(--bg-card);
            color: var(--text-primary);
            text-decoration: none;
            border-radius: 6px;
            border: 1px solid var(--border);
            font-size: 14px;
            font-weight: 500;
            transition: all 0.2s ease;
            flex: 1 1 auto;
        }}
        
        .btn:hover {{
            background: var(--border);
            border-color: var(--text-secondary);
        }}
        
        .btn-primary {{
            background: var(--accent);
            border-color: var(--accent);
            color: white;
        }}
        
        .btn-primary:hover {{
            background: var(--accent-hover);
            border-color: var(--accent-hover);
        }}
        
        .btn-danger {{
            background: var(--danger);
            border-color: var(--danger);
            color: white;
        }}
        
        .btn-danger:hover {{
            background: var(--danger-hover);
            border-color: var(--danger-hover);
        }}
        
        .user-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 16px;
        }}
        
        .user-card {{
            background: var(--bg-card);
            padding: 20px;
            border-radius: 10px;
            text-align: center;
            border: 1px solid var(--border);
            transition: transform 0.2s;
        }}
        
        .user-card:hover {{
            transform: translateY(-2px);
            border-color: var(--accent);
        }}
        
        .user-avatar {{
            font-size: 48px;
            margin-bottom: 12px;
        }}
        
        .user-name {{
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 4px;
        }}
        
        .user-role {{
            color: var(--text-secondary);
            font-size: 14px;
            margin-bottom: 16px;
        }}
        
        .login-btn {{
            width: 100%;
            padding: 8px;
            background: var(--accent);
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            text-decoration: none;
            display: inline-block;
        }}
        
        .login-btn:hover {{
            background: var(--accent-hover);
        }}
        
        .alert {{
            padding: 16px 20px;
            border-radius: 8px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
        }}
        
        .alert-success {{
            background: rgba(46, 160, 67, 0.15);
            border: 1px solid var(--accent);
            color: var(--accent);
        }}
        
        .footer {{
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid var(--border);
            text-align: center;
            color: var(--text-secondary);
            font-size: 14px;
        }}
        
        @media (max-width: 600px) {{
            .header {{
                flex-direction: column;
                gap: 16px;
                align-items: start;
            }}
            
            .stats-grid {{
                grid-template-columns: 1fr;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon"></span>
                <span class="logo-text">Session Manager</span>
            </div>"""
    
    @staticmethod
    def footer() -> str:
        return f"""        </div>
        <div class="footer">
            <p>Secure Session Demo • {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
    </div>
</body>
</html>"""
    
    @staticmethod
    def format_time(timestamp: float) -> str:
        """Format timestamp for display"""
        dt = datetime.fromtimestamp(timestamp)
        return dt.strftime('%Y-%m-%d %H:%M:%S')

# Initialize session manager
session_manager = SessionManager()

# Parse request
query = parse_qs(os.environ.get('QUERY_STRING', ''))
action = query.get('action', ['view'])[0]
username = query.get('username', [''])[0]

# Get session from cookie
def get_cookie(name):
    for item in os.environ.get('HTTP_COOKIE', '').split(';'):
        if '=' in item:
            k, v = item.strip().split('=', 1)
            if k == name:
                return v
    return ''

session_id = get_cookie(COOKIE_NAME)
session_data = session_manager.get(session_id) if session_id else None

# Handle actions
if action == 'login' and username:
    session_id = session_manager.create(username)
    session_data = session_manager.get(session_id)
    
    print("Content-Type: text/html")
    print(f"Set-Cookie: {COOKIE_NAME}={session_id}; Path=/; HttpOnly; SameSite=Lax")
    print()
    
    html = HTMLRenderer()
    print(html.header("Login Successful"))
    print(f"""
        <div class="status-badge">
            <span>🟢 Active Session</span>
            <span style="color: {next((u['color'] for u in DEMO_USERS if u['username'] == username), '#2ea043')};">{session_data['username']}</span>
        </div>
    </div>
    
    <div class="alert alert-success">
        <span style="font-size: 24px;">✓</span>
        <div style="flex: 1">
            <strong>Welcome, {username}!</strong>
            <p style="margin-top: 4px; opacity: 0.9;">Your session has been created successfully.</p>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">Session Overview</span>
            <span class="stat-value">New Session</span>
        </div>
        <div class="stats-grid">
            <div class="stat-item">
                <div class="stat-label">Username</div>
                <div class="stat-value">{session_data['username']}</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Session ID</div>
                <div class="stat-value">{session_id[:16]}...</div>
            </div>
            <div class="stat-item">
                <div class="stat-label">Created</div>
                <div class="stat-value">{html.format_time(session_data['created'])}</div>
            </div>
        </div>
    </div>
    
    <div class="btn-group">
        <a href="/cgi-bin/session.py" class="btn btn-primary">View Dashboard</a>
        <a href="/cgi-bin/" class="btn">Test Center</a>
    </div>
    """)
    print(html.footer())

elif action == 'logout':
    if session_id:
        session_manager.delete(session_id)
    
    print("Content-Type: text/html")
    print(f"Set-Cookie: {COOKIE_NAME}=; Max-Age=0; Path=/")
    print()
    
    html = HTMLRenderer()
    print(html.header("Logged Out"))
    print("""
        <div class="status-badge">
            <span>⚪ Not Logged In</span>
        </div>
    </div>
    
    <div class="alert alert-success">
        <span style="font-size: 24px;">👋</span>
        <div>
            <strong>You've been logged out</strong>
            <p style="margin-top: 4px; opacity: 0.9;">Your session has been terminated.</p>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">Return Options</span>
        </div>
        <div class="btn-group">
            <a href="/cgi-bin/session.py" class="btn btn-primary">Login Again</a>
            <a href="/cgi-bin/" class="btn">Test Center</a>
            <a href="/" class="btn">Home</a>
        </div>
    </div>
    """)
    print(html.footer())

else:
    print("Content-Type: text/html")
    print()
    
    html = HTMLRenderer()
    
    if session_data:
        # Active session view
        user_color = next((u['color'] for u in DEMO_USERS if u['username'] == session_data['username']), '#2ea043')
        
        print(html.header("Dashboard"))
        print(f"""
        <div class="status-badge">
            <span>🟢 Active Session</span>
            <span style="color: {user_color};">{session_data['username']}</span>
        </div>
    </div>
    
    <div class="stats-grid">
        <div class="stat-item">
            <div class="stat-label">Username</div>
            <div class="stat-value" style="color: {user_color};">{session_data['username']}</div>
        </div>
        <div class="stat-item">
            <div class="stat-label">Session Created</div>
            <div class="stat-value">{html.format_time(session_data['created'])}</div>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">Session Details</span>
        </div>
        <div class="info-row">
            <span class="info-label">Session ID</span>
            <span class="info-value">{session_id[:24]}...</span>
        </div>
        <div class="info-row">
            <span class="info-label">IP Address</span>
            <span class="info-value">{session_data.get('ip', 'Unknown')}</span>
        </div>
        <div class="info-row">
            <span class="info-label">User Agent</span>
            <span class="info-value">{session_data.get('user_agent', 'Unknown')[:40]}...</span>
        </div>
    </div>
    
    <div class="btn-group">
        <a href="?action=logout" class="btn btn-danger">Logout</a>
        <a href="/cgi-bin/" class="btn">Test Center</a>
        <a href="/" class="btn">Home</a>
    </div>
    """)
        print(html.footer())
    
    else:
        # No active session - show login page
        print(html.header("Login"))
        print("""
        <div class="status-badge">
            <span>⚪ Not Logged In</span>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">Select a User</span>
        </div>
        <p style="color: var(--text-secondary); margin-bottom: 20px;">
            Choose a demo user to start your session:
        </p>
        <div class="user-grid">
        """)
        
        for user in DEMO_USERS:
            print(f"""
            <div class="user-card">
                <div class="user-avatar">{user['avatar']}</div>
                <div class="user-name" style="color: {user['color']};">{user['username']}</div>
                <div class="user-role">{user['role']}</div>
                <a href="?action=login&username={user['username']}" class="login-btn" style="background: {user['color']};">Login</a>
            </div>
            """)
        
        print("""
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">Features</span>
        </div>
        <ul style="color: var(--text-secondary); list-style: none;">
            <li style="margin-bottom: 10px;">✓ Secure HTTP-only cookies</li>
            <li style="margin-bottom: 10px;">✓ IP address logging</li>
            <li style="margin-bottom: 10px;">✓ User agent tracking</li>
        </ul>
    </div>
    
    <div class="btn-group">
        <a href="/cgi-bin/" class="btn">Test Center</a>
        <a href="/" class="btn">Home</a>
    </div>
    """)
        print(html.footer())