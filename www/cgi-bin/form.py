#!/usr/bin/env python3
"""
Robust CGI Form Handler
- Safe HTML escaping
- GET and POST unified parsing
- Handles urlencoded and multipart/form-data
- Size limits
- Modern UI matching session/cookie/error demos
"""

import os
import sys
import html
import cgi
import cgitb
from datetime import datetime
from urllib.parse import parse_qs

# Enable CGI error reporting (development only)
cgitb.enable()

MAX_POST_SIZE = 1024 * 1024  # 1MB limit

class HTMLRenderer:
    """Handles HTML rendering with consistent UI"""
    
    @staticmethod
    def header(title: str) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - Form Handler</title>
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
            --warning-hover: #f0b400;
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
            background: linear-gradient(45deg, #58a6ff, #2ea043);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }}
        
        .method-badge {{
            background: var(--bg-secondary);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .method-tag {{
            background: var(--info);
            color: white;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
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
            color: var(--accent);
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
        
        .data-table {{
            width: 100%;
            border-collapse: collapse;
        }}
        
        .data-table th {{
            text-align: left;
            padding: 12px;
            background: var(--bg-card);
            color: var(--text-secondary);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            border-bottom: 1px solid var(--border);
        }}
        
        .data-table td {{
            padding: 12px;
            border-bottom: 1px solid var(--border);
            color: var(--text-primary);
        }}
        
        .data-table tr:hover td {{
            background: var(--bg-card);
        }}
        
        .field-name {{
            font-weight: 600;
            color: var(--info);
        }}
        
        .field-value {{
            font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace;
            color: var(--accent);
            background: var(--bg-primary);
            padding: 4px 10px;
            border-radius: 4px;
            border: 1px solid var(--border);
            display: inline-block;
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
        
        .alert-info {{
            background: rgba(88, 166, 255, 0.15);
            border: 1px solid var(--info);
            color: var(--info);
        }}
        
        .form-group {{
            margin-bottom: 20px;
        }}
        
        .form-label {{
            display: block;
            margin-bottom: 8px;
            color: var(--text-secondary);
            font-size: 14px;
            font-weight: 500;
        }}
        
        .form-input {{
            width: 100%;
            padding: 12px;
            background: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: 6px;
            color: var(--text-primary);
            font-size: 14px;
            transition: all 0.2s ease;
        }}
        
        .form-input:focus {{
            outline: none;
            border-color: var(--info);
            box-shadow: 0 0 0 3px rgba(88, 166, 255, 0.1);
        }}
        
        .form-textarea {{
            width: 100%;
            padding: 12px;
            background: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: 6px;
            color: var(--text-primary);
            font-size: 14px;
            font-family: inherit;
            resize: vertical;
            min-height: 100px;
            transition: all 0.2s ease;
        }}
        
        .form-textarea:focus {{
            outline: none;
            border-color: var(--info);
            box-shadow: 0 0 0 3px rgba(88, 166, 255, 0.1);
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
            cursor: pointer;
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
        
        .btn-info {{
            background: var(--info);
            border-color: var(--info);
            color: white;
        }}
        
        .btn-info:hover {{
            background: #6cb4ff;
            border-color: #6cb4ff;
        }}
        
        .badge {{
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
            display: inline-block;
        }}
        
        .badge-accent {{
            background: rgba(46, 160, 67, 0.2);
            color: var(--accent);
            border: 1px solid var(--accent);
        }}
        
        .badge-info {{
            background: rgba(88, 166, 255, 0.2);
            color: var(--info);
            border: 1px solid var(--info);
        }}
        
        .link-list {{
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
            margin-top: 16px;
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
            
            .stats-grid {{
                grid-template-columns: 1fr;
            }}
            
            .btn {{
                width: 100%;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon"></span>
                <span class="logo-text">Form Handler</span>
            </div>"""
    
    @staticmethod
    def footer() -> str:
        return f"""        </div>
        <div class="footer">
            <p>CGI Form Handler • GET & POST • URLEncoded & Multipart • {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
            <p style="margin-top: 8px; font-size: 12px; color: var(--text-secondary);">Max POST size: {MAX_POST_SIZE//1024}KB</p>
        </div>
    </div>
</body>
</html>"""


# ------------------------
# Helpers
# ------------------------

def h(text):
    """HTML escape"""
    return html.escape(str(text), quote=True)


def get_content_length() -> int:
    """Get content length from environment"""
    cl = os.environ.get("CONTENT_LENGTH")
    return int(cl) if cl and cl.strip() else 0


def get_request_method() -> str:
    """Get request method"""
    return os.environ.get("REQUEST_METHOD", "GET")


def show_table(fields):
    """Display form fields in a table"""
    total_fields = len(fields.keys())
    
    print(f"""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Submitted Data</span>
            <span class="badge badge-accent">{total_fields} field{'s' if total_fields != 1 else ''}</span>
        </div>
        <table class="data-table">
            <thead>
                <tr>
                    <th>Field Name</th>
                    <th>Value</th>
                </tr>
            </thead>
            <tbody>""")
    
    for key in fields.keys():
        values = fields.getlist(key)
        for v in values:
            print(f"""
                <tr>
                    <td><span class="field-name">{h(key)}</span></td>
                    <td><span class="field-value">{h(v)}</span></td>
                </tr>""")
    
    print("""
            </tbody>
        </table>
    </div>""")


# ------------------------
# Main
# ------------------------

# Initialize renderer
html_renderer = HTMLRenderer()

# Parse request data
method = get_request_method()
content_length = get_content_length()

# Check size limit
if method == "POST" and content_length > MAX_POST_SIZE:
    print_headers()
    print(html_renderer.header("Error"))
    print(f"""
        <div class="method-badge">
            <span class="method-tag">{method}</span>
            <span>Size: {content_length} bytes</span>
        </div>
    </div>
    
    <div class="alert alert-danger">
        <span style="font-size: 24px;">❌</span>
        <div style="flex: 1">
            <strong>POST body too large</strong>
            <p style="margin-top: 4px; opacity: 0.9;">Maximum allowed: {MAX_POST_SIZE//1024}KB, Received: {content_length//1024}KB</p>
        </div>
    </div>
    
    <div class="card">
        <div class="card-header">
            <span class="card-title">⚙️ Navigation</span>
        </div>
        <div class="link-list">
            <a href="/cgi-bin/form.py" class="btn btn-primary">Try Again</a>
            <a href="/cgi-bin/" class="btn">Test Center</a>
            <a href="/" class="btn">Home</a>
        </div>
    </div>""")
    print(html_renderer.footer())
    sys.exit(0)

# Parse form data
try:
    form = cgi.FieldStorage()
    parse_error = None
except Exception as e:
    form = {}
    parse_error = str(e)

# Start response
print("Content-Type: text/html; charset=utf-8")
print()

# Start output
print(html_renderer.header("Form Handler"))

# Method badge
print(f"""
        <div class="method-badge">
            <span class="method-tag">{method}</span>
            <span>{content_length} bytes</span>
        </div>
    </div>""")

# Stats grid
print("""
    <div class="stats-grid">""")

print(f"""
        <div class="stat-card">
            <div class="stat-title">Request Method</div>
            <div class="stat-number" style="color: var(--info);">{method}</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Content Length</div>
            <div class="stat-number">{content_length}</div>
            <div class="stat-unit">bytes</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Fields</div>
            <div class="stat-number">{len(form) if form else 0}</div>
            <div class="stat-unit">submitted</div>
        </div>""")

print("""
    </div>""")

# Parse error alert
if parse_error:
    print(f"""
    <div class="alert alert-danger">
        <span style="font-size: 24px;">⚠️</span>
        <div style="flex: 1">
            <strong>Form Parsing Error</strong>
            <p style="margin-top: 4px; opacity: 0.9;">{h(parse_error)}</p>
        </div>
    </div>""")

# No parameters
elif not form or len(form) == 0:
    print("""
    <div class="alert alert-info">
        <div style="flex: 1">
            <strong>No Parameters Received</strong>
            <p style="margin-top: 4px; opacity: 0.9;">Submit the form below or use the GET test links.</p>
        </div>
    </div>""")

# Show form data
else:
    show_table(form)

# Test Form
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Test Form</span>
            <span class="badge badge-info">POST</span>
        </div>
        <form method="POST" action="/cgi-bin/form.py">
            <div class="form-group">
                <label class="form-label" for="name">Name</label>
                <input type="text" class="form-input" id="name" name="name" value="Test User" placeholder="Enter your name">
            </div>
            
            <div class="form-group">
                <label class="form-label" for="email">Email</label>
                <input type="email" class="form-input" id="email" name="email" value="test@example.com" placeholder="Enter your email">
            </div>
            
            <div class="form-group">
                <label class="form-label" for="message">Message</label>
                <textarea class="form-textarea" id="message" name="message" placeholder="Type your message here...">Hello CGI</textarea>
            </div>
            
            <div style="display: flex; gap: 12px;">
                <button type="submit" class="btn btn-primary">Submit Form</button>
            </div>
        </form>
    </div>""")

# Quick Test Links
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Quick Tests</span>
            <span class="badge badge-info">GET</span>
        </div>
        <div class="link-list">
            <a href="/cgi-bin/form.py?x=42&y=hello" class="btn btn-info">Test GET (x=42, y=hello)</a>
            <a href="/cgi-bin/form.py?name=John&age=25&city=New+York" class="btn btn-info">User Data</a>
            <a href="/cgi-bin/form.py?product=123&quantity=2&discount=10" class="btn btn-info">Order Data</a>
        </div>
    </div>""")

# Navigation
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Navigation</span>
        </div>
        <div class="link-list">
            <a href="/cgi-bin/form.py" class="btn">Reset Page</a>
            <a href="/cgi-bin/" class="btn">Test Center</a>
            <a href="/" class="btn">Home</a>
        </div>
    </div>""")

print(html_renderer.footer())