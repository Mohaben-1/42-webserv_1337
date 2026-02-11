#!/usr/bin/env python3
"""
Advanced Cookie Management Demo
A clean, organized CGI script with modern UI and enhanced features
"""
import os
import time
from urllib.parse import parse_qs, quote, unquote
from datetime import datetime
from typing import Dict, List, Tuple

# Configuration
PRESET_COOKIES = [
    {"name": "Name:", "value": "Medd", "description": "User identity", "color": "#FFFFFF"},
    {"name": "Country:", "value": "Morocco", "description": "Location", "color": "#FFFFFF"},
    {"name": "Language:", "value": "Ar", "description": "Language", "color": "#FFFFFF"},
    {"name": "Timezone:", "value": "UTC+1", "description": "Time zone", "color": "#FFFFFF"},
]

def get_cookies() -> Dict[str, str]:
    """Parse and return all cookies from the request"""
    cookies = {}
    cookie_header = os.environ.get('HTTP_COOKIE', '')
    
    for item in cookie_header.split(';'):
        if '=' in item:
            key, value = item.strip().split('=', 1)
            try:
                # Try to decode URL-encoded values
                cookies[key] = unquote(value)
            except:
                cookies[key] = value
    return cookies

def set_cookie(name: str, value: str) -> str:
    """Generate a Set-Cookie header (session cookie)"""
    encoded_value = quote(value)
    return f"Set-Cookie: {name}={encoded_value}; Path=/; HttpOnly; SameSite=Lax"

def delete_cookie(name: str) -> str:
    """Generate a Set-Cookie header to delete a cookie"""
    return f"Set-Cookie: {name}=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax"

class HTMLRenderer:
    """Handles HTML rendering with consistent UI"""
    
    @staticmethod
    def header(title: str) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - Cookie Demo</title>
    <style>
        :root {{
            --bg-primary: #0a0c10;
            --bg-secondary: #161b22;
            --bg-card: #21262d;
            --accent: #2ea043;
            --accent-hover: #3fb950;
            --danger: #da3633;
            --danger-hover: #f85149;
            --warning: #e3b341;
            --info: #58a6ff;
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
            max-width: 900px;
            margin: 0 auto;
        }}
        
        .header {{
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 40px;
            padding-bottom: 20px;
            border-bottom: 1px solid var(--border);
            flex-wrap: wrap;
            gap: 16px;
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
            background: linear-gradient(45deg, #f778ba, #58a6ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }}
        
        .cookie-count {{
            background: var(--bg-secondary);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .cookie-badge {{
            background: var(--accent);
            color: white;
            padding: 2px 8px;
            border-radius: 12px;
            font-size: 12px;
            margin-left: 8px;
        }}
        
        .stats-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }}
        
        .stat-card {{
            background: var(--bg-secondary);
            border-radius: 12px;
            padding: 20px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow);
        }}
        
        .stat-title {{
            color: var(--text-secondary);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            gap: 6px;
        }}
        
        .stat-number {{
            font-size: 32px;
            font-weight: 600;
            color: var(--text-primary);
        }}
        
        .stat-unit {{
            font-size: 14px;
            color: var(--text-secondary);
            margin-left: 4px;
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
            flex-wrap: wrap;
            gap: 12px;
        }}
        
        .card-title {{
            font-size: 18px;
            font-weight: 600;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .cookie-table {{
            width: 100%;
            border-collapse: collapse;
        }}
        
        .cookie-row {{
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 16px;
            background: var(--bg-card);
            border-radius: 8px;
            margin-bottom: 8px;
            border: 1px solid var(--border);
            transition: all 0.2s ease;
        }}
        
        .cookie-row:hover {{
            border-color: var(--accent);
            transform: translateX(4px);
        }}
        
        .cookie-info {{
            display: flex;
            flex-direction: column;
            gap: 4px;
        }}
        
        .cookie-name {{
            font-size: 16px;
            font-weight: 600;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .cookie-name-badge {{
            background: var(--bg-primary);
            padding: 2px 8px;
            border-radius: 12px;
            font-size: 11px;
            color: var(--text-secondary);
            font-weight: normal;
        }}
        
        .cookie-value {{
            font-family: 'SF Mono', Monaco, 'Cascadia Code', 'Roboto Mono', monospace;
            font-size: 14px;
            color: var(--accent);
            background: var(--bg-primary);
            padding: 6px 12px;
            border-radius: 6px;
            border: 1px solid var(--border);
            display: inline-block;
        }}
        
        .cookie-meta {{
            font-size: 12px;
            color: var(--text-secondary);
            display: flex;
            gap: 16px;
            margin-top: 4px;
        }}
        
        .cookie-actions {{
            display: flex;
            gap: 8px;
        }}
        
        .btn {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 6px;
            padding: 8px 16px;
            background: var(--bg-card);
            color: var(--text-primary);
            text-decoration: none;
            border-radius: 6px;
            border: 1px solid var(--border);
            font-size: 13px;
            font-weight: 500;
            transition: all 0.2s ease;
            cursor: pointer;
        }}
        
        .btn:hover {{
            background: var(--border);
            border-color: var(--text-secondary);
        }}
        
        .btn-sm {{
            padding: 6px 12px;
            font-size: 12px;
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
        
        .btn-warning {{
            background: var(--warning);
            border-color: var(--warning);
            color: white;
        }}
        
        .btn-warning:hover {{
            background: #f0b400;
            border-color: #f0b400;
        }}
        
        .btn-info {{
            background: var(--info);
            border-color: var(--info);
            color: white;
        }}
        
        .cookie-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
            gap: 16px;
        }}
        
        .preset-card {{
            background: var(--bg-card);
            border-radius: 10px;
            padding: 20px;
            border: 1px solid var(--border);
            transition: all 0.2s ease;
        }}
        
        .preset-card:hover {{
            transform: translateY(-2px);
            border-color: var(--accent);
            box-shadow: var(--shadow);
        }}
        
        .preset-icon {{
            font-size: 28px;
            margin-bottom: 12px;
        }}
        
        .preset-name {{
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 4px;
        }}
        
        .preset-description {{
            color: var(--text-secondary);
            font-size: 13px;
            margin-bottom: 16px;
        }}
        
        .preset-value {{
            font-family: monospace;
            font-size: 14px;
            background: var(--bg-primary);
            padding: 6px 12px;
            border-radius: 4px;
            margin-bottom: 16px;
            display: inline-block;
            border: 1px solid var(--border);
        }}
        
        .alert {{
            padding: 16px 20px;
            border-radius: 8px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
            animation: slideIn 0.3s ease;
        }}
        
        @keyframes slideIn {{
            from {{
                opacity: 0;
                transform: translateY(-10px);
            }}
            to {{
                opacity: 1;
                transform: translateY(0);
            }}
        }}
        
        .alert-success {{
            background: rgba(46, 160, 67, 0.15);
            border: 1px solid var(--accent);
            color: var(--accent);
        }}
        
        .alert-warning {{
            background: rgba(227, 179, 65, 0.15);
            border: 1px solid var(--warning);
            color: var(--warning);
        }}
        
        .alert-danger {{
            background: rgba(218, 54, 51, 0.15);
            border: 1px solid var(--danger);
            color: var(--danger);
        }}
        
        .empty-state {{
            text-align: center;
            padding: 48px 24px;
            background: var(--bg-card);
            border-radius: 12px;
            border: 2px dashed var(--border);
        }}
        
        .empty-state-icon {{
            font-size: 48px;
            margin-bottom: 16px;
            opacity: 0.5;
        }}
        
        .empty-state-title {{
            font-size: 20px;
            font-weight: 600;
            margin-bottom: 8px;
        }}
        
        .empty-state-text {{
            color: var(--text-secondary);
            margin-bottom: 24px;
        }}
        
        .badge {{
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
        }}
        
        .badge-success {{
            background: rgba(46, 160, 67, 0.2);
            color: var(--accent);
            border: 1px solid var(--accent);
        }}
        
        .divider {{
            margin: 24px 0;
            border: none;
            border-top: 1px solid var(--border);
        }}
        
        .footer {{
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid var(--border);
            text-align: center;
            color: var(--text-secondary);
            font-size: 13px;
        }}
        
        @media (max-width: 600px) {{
            .header {{
                flex-direction: column;
                align-items: start;
            }}
            
            .cookie-row {{
                flex-direction: column;
                align-items: start;
                gap: 12px;
            }}
            
            .cookie-actions {{
                width: 100%;
                justify-content: flex-end;
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
                <span class="logo-text">Cookie Manager</span>
            </div>"""
    
    @staticmethod
    def footer() -> str:
        return f"""        </div>
        <div class="footer">
            <p>Secure Cookie Demo • HTTP-Only • SameSite=Lax • {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>
    </div>
</body>
</html>"""
    
    @staticmethod
    def format_size(size: int) -> str:
        """Format cookie size"""
        if size < 1024:
            return f"{size} B"
        return f"{size/1024:.1f} KB"

# Parse request
query = parse_qs(os.environ.get('QUERY_STRING', ''), keep_blank_values=True)
cookies = get_cookies()

# Track messages and headers
messages = []
headers = []
action_performed = False

# Handle single cookie deletion
if 'delete' in query:
    cookie_name = query.get('delete', [''])[0]
    if cookie_name and cookie_name in cookies:
        headers.append(delete_cookie(cookie_name))
        action_performed = True
        # Update cookies dict for display
        del cookies[cookie_name]

# Handle clear all cookies
elif 'clear' in query:
    for name in cookies.keys():
        headers.append(delete_cookie(name))
    action_performed = True
    cookies = {}

# Handle set cookie
elif query and not any(x in query for x in ['delete', 'clear']):
    for name, values in query.items():
        if name in ('custom_name', 'custom_value'):
            continue  # Skip — handled by the custom cookie block below
        if name and values and values[0] is not None:
            value = values[0]
            headers.append(set_cookie(name, value))
            cookies[name] = value
            action_performed = True

# Handle custom cookie form
if 'custom_name' in query and 'custom_value' in query:
    custom_name = query.get('custom_name', [''])[0]
    custom_value = query.get('custom_value', [''])[0]
    if custom_name and custom_value:
        headers.append(set_cookie(custom_name, custom_value))
        cookies[custom_name] = custom_value
        action_performed = True

# Start response
print("Content-Type: text/html")
for header in headers:
    print(header)
print()

# Initialize renderer
html = HTMLRenderer()

# Calculate stats
total_cookies = len(cookies)
total_size = sum(len(k) + len(v) for k, v in cookies.items())

# Start output
print(html.header("Cookie Manager"))

# Cookie count badge
print(f"""
        <div class="cookie-count">
            <span>Active Cookies</span>
            <span class="cookie-badge">{total_cookies}</span>
        </div>
    </div>""")

# Display messages
for msg_type, msg in messages:
    alert_class = {
        'success': 'alert-success',
        'warning': 'alert-warning',
        'danger': 'alert-danger'
    }.get(msg_type, 'alert-success')
    
    icon = {
        'success': '✅',
        'warning': '⚠️',
        'danger': '❌'
    }.get(msg_type, '✅')
    
    print(f"""
    <div class="alert {alert_class}">
        <span style="font-size: 20px;">{icon}</span>
        <div style="flex: 1">
            <strong>{msg}</strong>
        </div>
    </div>""")

# Statistics cards
print("""
    <div class="stats-grid">""")

print(f"""
        <div class="stat-card">
            <div class="stat-title">Total Cookies</div>
            <div class="stat-number">{total_cookies}</div>
            <div class="stat-unit">active</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Security</div>
            <div class="stat-number">HTTP</div>
            <div class="stat-unit">Only</div>
        </div>""")

print("""
    </div>""")

# Current Cookies Section
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Current Cookies</span>""")

if cookies:
    print(f"""
            <span class="badge badge-success">{total_cookies} items</span>
        </div>
        <div class="cookie-list">""")
    
    for name, value in cookies.items():
        print(f"""
            <div class="cookie-row">
                <div class="cookie-info">
                    <div class="cookie-name">
                        {name}
                    </div>
                    <div class="cookie-value">{value}</div>
                </div>
                <div class="cookie-actions">
                    <a href="?delete={name}" class="btn btn-sm btn-danger" onclick="return confirm('Delete cookie \\'{name}\\'?')">Delete</a>
                </div>
            </div>""")
    print("""        </div>""")
else:
    print("""
        </div>
        <div class="empty-state">
            <div class="empty-state-title">No Cookies Found</div>
            <div class="empty-state-text">Set a cookie using the presets below or create a custom one.</div>
        </div>""")

print("""
    </div>""")

# Preset Cookies Section
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Quick Presets</span>
        </div>
        <div class="cookie-grid">""")

for preset in PRESET_COOKIES:
    print(f"""
            <div class="preset-card">
                <div class="preset-name" style="color: {preset['color']};">{preset['name']}</div>
                <div class="preset-description">{preset['description']}</div>
                <div class="preset-value">{preset['value']}</div>
                <div style="display: flex; gap: 8px; margin-top: 12px;">
                    <a href="?{preset['name']}={preset['value']}" class="btn btn-sm btn-primary" style="flex: 1;">Set Cookie</a>
                </div>
            </div>""")

print("""
        </div>
    </div>""")

# Custom Cookie Form
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Custom Cookie</span>
            <span class="badge badge-success">manual</span>
        </div>
        <form method="get" style="display: flex; flex-direction: column; gap: 16px;">
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px;">
                <input type="text" 
                       name="custom_name" 
                       placeholder="Cookie name" 
                       style="padding: 12px; background: var(--bg-card); border: 1px solid var(--border); border-radius: 6px; color: var(--text-primary);"
                       required>
                <input type="text" 
                       name="custom_value" 
                       placeholder="Cookie value" 
                       style="padding: 12px; background: var(--bg-card); border: 1px solid var(--border); border-radius: 6px; color: var(--text-primary);"
                       required>
            </div>
            <div style="display: flex; gap: 12px;">
                <button type="submit" class="btn btn-primary" style="flex: 2;">Create Cookie</button>
            </div>
        </form>
    </div>""")

# Actions Section
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Actions</span>
        </div>
        <div style="display: flex; gap: 12px; flex-wrap: wrap;">
            <a href="?clear=1" class="btn btn-danger" onclick="return confirm('Clear all cookies?')">Clear All Cookies</a>
            <a href="/cgi-bin/" class="btn">Test Center</a>
            <a href="/" class="btn">Home</a>
        </div>
    </div>""")


print(html.footer())