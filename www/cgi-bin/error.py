#!/usr/bin/env python3
"""
CGI script for testing error handling
Advanced Error Testing Demo with Modern UI
"""
import os
import sys
import time
import urllib.parse
from datetime import datetime
from typing import Dict, Optional

# Error test configurations
ERROR_TESTS = [
    {
        "type": "exception",
        "name": "Raise Exception",
        "description": "Throws a Python exception",
        "color": "#f778ba",
        "severity": "high"
    },
    {
        "type": "exit",
        "name": "Exit Code 1",
        "description": "Exits with status code 1",
        "color": "#e3b341",
        "severity": "medium"
    },
    {
        "type": "divide",
        "name": "Division by Zero",
        "description": "Triggers ZeroDivisionError",
        "color": "#ff7b72",
        "severity": "high"
    },
    {
        "type": "timeout",
        "name": "Infinite Loop",
        "description": "Runs forever (timeout test)",
        "color": "#58a6ff",
        "severity": "critical"
    },
    {
        "type": "header",
        "name": "Invalid Header",
        "description": "Malformed HTTP header",
        "color": "#d2a8ff",
        "severity": "high"
    }
]

class HTMLRenderer:
    """Handles HTML rendering with consistent UI"""
    
    @staticmethod
    def header(title: str) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - Error Testing</title>
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
            --critical: #f778ba;
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
            max-width: 1000px;
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
            background: linear-gradient(45deg, #f778ba, #ff7b72);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }}
        
        .warning-badge {{
            background: rgba(227, 179, 65, 0.15);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--warning);
            color: var(--warning);
            display: flex;
            align-items: center;
            gap: 8px;
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
            color: var(--critical);
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
        
        .error-grid {{
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
            gap: 16px;
        }}
        
        .error-card {{
            background: var(--bg-card);
            border-radius: 12px;
            padding: 20px;
            border: 1px solid var(--border);
            transition: all 0.2s ease;
            position: relative;
            overflow: hidden;
        }}
        
        .error-card:hover {{
            transform: translateY(-2px);
            box-shadow: var(--shadow);
        }}
        
        .error-card.critical:hover {{
            border-color: var(--critical);
        }}
        
        .error-card.high:hover {{
            border-color: var(--danger);
        }}
        
        .error-card.medium:hover {{
            border-color: var(--warning);
        }}
        
        .error-card.low:hover {{
            border-color: var(--info);
        }}
        
        .severity-badge {{
            position: absolute;
            top: 12px;
            right: 12px;
            padding: 4px 10px;
            border-radius: 20px;
            font-size: 11px;
            font-weight: 600;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }}
        
        .severity-critical {{
            background: rgba(247, 120, 186, 0.15);
            color: var(--critical);
            border: 1px solid var(--critical);
        }}
        
        .severity-high {{
            background: rgba(218, 54, 51, 0.15);
            color: var(--danger);
            border: 1px solid var(--danger);
        }}
        
        .severity-medium {{
            background: rgba(227, 179, 65, 0.15);
            color: var(--warning);
            border: 1px solid var(--warning);
        }}
        
        .severity-low {{
            background: rgba(88, 166, 255, 0.15);
            color: var(--info);
            border: 1px solid var(--info);
        }}
        
        .error-name {{
            font-size: 18px;
            font-weight: 600;
            margin-bottom: 6px;
            margin-top: 8px;
        }}
        
        .error-description {{
            color: var(--text-secondary);
            font-size: 13px;
            margin-bottom: 20px;
            line-height: 1.5;
        }}
        
        .error-type {{
            font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace;
            font-size: 12px;
            color: var(--text-secondary);
            background: var(--bg-primary);
            padding: 4px 10px;
            border-radius: 4px;
            display: inline-block;
            border: 1px solid var(--border);
            margin-bottom: 16px;
        }}
        
        .btn {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 6px;
            padding: 10px 20px;
            background: var(--bg-card);
            color: var(--text-primary);
            text-decoration: none;
            border-radius: 6px;
            border: 1px solid var(--border);
            font-size: 14px;
            font-weight: 500;
            transition: all 0.2s ease;
            cursor: pointer;
            width: 100%;
        }}
        
        .btn:hover {{
            background: var(--border);
            border-color: var(--text-secondary);
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
            background: var(--warning-hover);
            border-color: var(--warning-hover);
        }}
        
        .btn-critical {{
            background: var(--critical);
            border-color: var(--critical);
            color: white;
        }}
        
        .btn-critical:hover {{
            background: #ff99cc;
            border-color: #ff99cc;
        }}
        
        .alert {{
            padding: 16px 20px;
            border-radius: 8px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
            background: rgba(227, 179, 65, 0.1);
            border: 1px solid var(--warning);
            color: var(--warning);
        }}
        
        .badge {{
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
        }}
        
        .badge-critical {{
            background: rgba(247, 120, 186, 0.2);
            color: var(--critical);
            border: 1px solid var(--critical);
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
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon">⚠️</span>
                <span class="logo-text">Error Testing</span>
            </div>"""
    
    @staticmethod
    def footer() -> str:
        return f"""        </div>
        <div class="footer">
            <p>CGI Error Testing Suite • Use for development/testing only • {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
            <p style="margin-top: 8px; font-size: 12px; color: var(--warning);">⚠️ These endpoints will intentionally crash - do not use in production</p>
        </div>
    </div>
</body>
</html>"""

# Parse request
query_string = os.environ.get('QUERY_STRING', '')
params = dict(urllib.parse.parse_qsl(query_string))
error_type = params.get('type', '')

# Handle error tests
if error_type:
    # Flush output before error for some tests
    if error_type in ['exit', 'timeout']:
        print("Content-Type: text/html")
        print("")
        sys.stdout.flush()
    
    # Execute error test
    if error_type == 'exit':
        print("<h1>Exiting with code 1...</h1>")
        sys.stdout.flush()
        sys.exit(1)
    
    elif error_type == 'exception':
        print("Content-Type: text/html")
        print("")
        raise Exception("Test exception raised from error.py")
    
    elif error_type == 'divide':
        print("Content-Type: text/html")
        print("")
        x = 1 / 0  # ZeroDivisionError
    
    elif error_type == 'timeout':
        print("<h1>Infinite Loop Started</h1>")
        print("<p>This script will run forever until timeout...</p>")
        print("<p>Check server configuration for CGI timeout settings.</p>")
        sys.stdout.flush()
        while True:
            time.sleep(10)
    
    elif error_type == 'header':
        # Send malformed header
        print("This is not a valid HTTP header")
        print("")
        print("<h1>Invalid Header Test</h1>")
    
    else:
        # Unknown error type - show normal page
        pass

# Normal page - show error testing dashboard
if not error_type or error_type not in [t['type'] for t in ERROR_TESTS]:
    # Initialize renderer
    html = HTMLRenderer()
    
    # Calculate stats
    total_tests = len(ERROR_TESTS)
    critical_count = sum(1 for t in ERROR_TESTS if t['severity'] == 'critical')
    high_count = sum(1 for t in ERROR_TESTS if t['severity'] == 'high')
    
    # Start output
    print("Content-Type: text/html")
    print()
    print(html.header("Error Testing"))
    
    # Warning badge
    print("""
        <div class="warning-badge">
            <span>⚠️ TESTING MODE</span>
            <span>Development Only</span>
        </div>
    </div>""")
    
    # Alert message
    print("""
    <div class="alert">
        <span style="font-size: 24px;">⚠️</span>
        <div style="flex: 1">
            <strong>Development & Testing Only</strong>
            <p style="margin-top: 4px; opacity: 0.9;">These endpoints are designed to crash - use for testing server error handling.</p>
        </div>
    </div>""")
    
    # Statistics cards
    print("""
    <div class="stats-grid">""")
    
    print(f"""
        <div class="stat-card">
            <div class="stat-title">Test Cases</div>
            <div class="stat-number">{total_tests}</div>
            <div class="stat-unit">available</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Critical</div>
            <div class="stat-number" style="color: var(--critical);">{critical_count}</div>
            <div class="stat-unit">tests</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">High Risk</div>
            <div class="stat-number" style="color: var(--danger);">{high_count}</div>
            <div class="stat-unit">tests</div>
        </div>""")
    
    print("""
    </div>""")
    
    # Main error tests card
    print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Error Test Cases</span>
            <span class="badge badge-critical">click to crash</span>
        </div>
        <div class="error-grid">""")
    
    for test in ERROR_TESTS:
        severity_class = {
            'critical': 'critical',
            'high': 'high',
            'medium': 'medium',
            'low': 'low'
        }.get(test['severity'], 'low')
        
        btn_class = {
            'critical': 'btn-critical',
            'high': 'btn-danger',
            'medium': 'btn-warning',
            'low': 'btn-info'
        }.get(test['severity'], 'btn')
        
        print(f"""
            <div class="error-card {severity_class}">
                <span class="severity-badge severity-{test['severity']}">{test['severity']}</span>
                <div class="error-name" style="color: {test['color']};">{test['name']}</div>
                <div class="error-description">{test['description']}</div>
                <div class="error-type">{test['type']}</div>
                <a href="?type={test['type']}" class="btn {btn_class}" onclick="return confirm('⚠️ This will crash the script! Are you sure?')">
                    Trigger Error
                </a>
            </div>""")
    
    print("""
        </div>
    </div>""")
    

    
    # Actions section
    print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Navigation</span>
        </div>
        <div style="display: flex; gap: 12px; flex-wrap: wrap;">
            <a href="/cgi-bin/" class="btn" style="flex: 1;">Test Center</a>
            <a href="/" class="btn" style="flex: 1;">Home</a>
        </div>
        <p style="color: var(--text-secondary); font-size: 12px; margin-top: 16px; text-align: center;">
            After triggering an error, use browser back button or navigation links to return here.
        </p>
    </div>""")
    
    print(html.footer())