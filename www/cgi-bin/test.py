#!/usr/bin/env python3
"""
CGI Test Script - Environment Info
Modern UI matching session/cookie/error demos
"""
import os
import sys
from datetime import datetime

class HTMLRenderer:
    """Handles HTML rendering with consistent UI"""
    
    @staticmethod
    def header(title: str) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title} - CGI Test</title>
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
            background: linear-gradient(45deg, #58a6ff, #2ea043);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }}
        
        .python-badge {{
            background: var(--bg-secondary);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        
        .python-tag {{
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
            font-size: 24px;
            font-weight: 600;
            color: var(--accent);
            font-family: monospace;
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
        
        .env-table {{
            width: 100%;
            border-collapse: collapse;
            font-size: 13px;
        }}
        
        .env-table tr {{
            border-bottom: 1px solid var(--border);
        }}
        
        .env-table tr:last-child {{
            border-bottom: none;
        }}
        
        .env-table td {{
            padding: 12px 8px;
            vertical-align: top;
        }}
        
        .env-table td:first-child {{
            color: var(--text-secondary);
            font-weight: 500;
            width: 30%;
            font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace;
            white-space: nowrap;
        }}
        
        .env-table td:last-child {{
            color: var(--info);
            font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace;
            word-break: break-word;
        }}
        
        .env-table tr:hover td {{
            background: var(--bg-card);
        }}
        
        .success-badge {{
            display: inline-block;
            padding: 4px 12px;
            background: rgba(46, 160, 67, 0.15);
            color: var(--accent);
            border: 1px solid var(--accent);
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
        }}
        
        .value-not-set {{
            color: var(--text-secondary) !important;
            font-style: italic;
        }}
        
        .btn {{
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            padding: 10px 20px;
            background: var(--bg-card);
            color: var(--text-primary);
            text-decoration: none;
            border-radius: 6px;
            border: 1px solid var(--border);
            font-size: 14px;
            font-weight: 500;
            transition: all 0.2s ease;
            margin-right: 12px;
            margin-bottom: 8px;
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
        
        .btn-group {{
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
            margin-top: 16px;
        }}
        
        .badge {{
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
            display: inline-block;
        }}
        
        .badge-python {{
            background: rgba(88, 166, 255, 0.2);
            color: var(--info);
            border: 1px solid var(--info);
        }}
        
        .badge-cgi {{
            background: rgba(46, 160, 67, 0.2);
            color: var(--accent);
            border: 1px solid var(--accent);
        }}
        
        .search-box {{
            width: 100%;
            padding: 12px;
            background: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: 6px;
            color: var(--text-primary);
            font-size: 14px;
            margin-bottom: 20px;
        }}
        
        .search-box:focus {{
            outline: none;
            border-color: var(--info);
            box-shadow: 0 0 0 3px rgba(88, 166, 255, 0.1);
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
            
            .env-table td:first-child {{
                white-space: normal;
                width: 40%;
            }}
            
            .btn {{
                width: 100%;
                margin-right: 0;
            }}
        }}
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon"></span>
                <span class="logo-text">CGI Test</span>
            </div>"""
    
    @staticmethod
    def footer() -> str:
        return f"""        </div>
        <div class="footer">
            <p>Python CGI Test Script • Environment Inspector • {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
            <p style="margin-top: 8px; font-size: 12px; color: var(--text-secondary);">Python {sys.version.split()[0]} • CGI/1.1 • {os.environ.get('SERVER_SOFTWARE', 'Unknown')}</p>
        </div>
    </div>
</body>
</html>"""


# ------------------------
# Helper Functions
# ------------------------

def get_python_version() -> str:
    """Get Python version string"""
    return sys.version.split()[0]

def get_cgi_vars() -> list:
    """List of relevant CGI environment variables"""
    return [
        'REQUEST_METHOD', 'QUERY_STRING', 'SCRIPT_NAME', 'SCRIPT_FILENAME', 'PATH_TRANSLATED',
        'SERVER_NAME', 'SERVER_PORT', 'SERVER_SOFTWARE', 'SERVER_PROTOCOL',
        'GATEWAY_INTERFACE', 'DOCUMENT_ROOT', 'HTTP_HOST', 'HTTP_USER_AGENT', 'HTTP_ACCEPT', 'HTTP_ACCEPT_LANGUAGE',
        'HTTP_ACCEPT_ENCODING', 'HTTP_CONNECTION'
    ]

def get_system_vars() -> list:
    """List of system environment variables"""
    return [
        'USER', 'HOME', 'PWD', 'SHELL', 'PATH', 'LANG', 'LC_ALL',
        'TZ', 'PYTHONPATH', 'PYTHONHOME'
    ]

def h(text):
    """HTML escape - simple version"""
    if text is None:
        return ''
    return str(text).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

# ------------------------
# Main
# ------------------------

# CGI scripts must output headers first, then a blank line, then content
print("Content-Type: text/html")
print("")  # Blank line separates headers from body

# Initialize renderer
html = HTMLRenderer()

# Start output
print(html.header("CGI Test"))

# Python badge
python_version = get_python_version()
print(f"""
        <div class="python-badge">
            <span class="python-tag">Python {python_version}</span>
            <span>CGI Environment</span>
        </div>
    </div>""")

# Stats grid
total_cgi_vars = len(get_cgi_vars())
total_system_vars = len(get_system_vars())
query_string = os.environ.get('QUERY_STRING', '<none>')
query_preview = query_string[:30] + '...' if len(query_string) > 30 else query_string

print(f"""
    <div class="stats-grid">
        <div class="stat-card">
            <div class="stat-title">Python Version</div>
            <div class="stat-number" style="color: var(--info);">{python_version}</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">CGI Variables</div>
            <div class="stat-number" style="color: var(--accent);">{total_cgi_vars}</div>
        </div>
        <div class="stat-card">
            <div class="stat-title">Request Method</div>
            <div class="stat-number" style="color: var(--warning);">{os.environ.get('REQUEST_METHOD', 'N/A')}</div>
        </div>
    </div>""")

# Success alert


# CGI Environment Variables Card
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">CGI Environment Variables</span>
            <span class="badge badge-python">RFC 3875</span>
        </div>
        <input type="text" id="cgiSearch" class="search-box" placeholder="🔍 Filter CGI variables..." onkeyup="filterTable('cgiTable', this.value)">
        <table class="env-table" id="cgiTable">""")

for var in get_cgi_vars():
    value = os.environ.get(var, '<not set>')
    value_class = 'value-not-set' if value == '<not set>' else ''
    print(f"""
            <tr>
                <td>{h(var)}</td>
                <td class="{value_class}">{h(value)}</td>
            </tr>""")

print("""
        </table>
    </div>""")


# HTTP Headers Card
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">HTTP Headers</span>
            <span class="badge badge-python">request</span>
        </div>
        <table class="env-table">""")

http_headers = [var for var in os.environ.keys() if var.startswith('HTTP_')]
for var in sorted(http_headers):
    value = os.environ.get(var, '')
    header_name = var[5:].replace('_', '-').title()
    print(f"""
            <tr>
                <td>{h(header_name)}</td>
                <td>{h(value)}</td>
            </tr>""")

if not http_headers:
    print("""
            <tr>
                <td colspan="2" style="color: var(--text-secondary); text-align: center; padding: 20px;">
                    No HTTP headers received
                </td>
            </tr>""")

print("""
        </table>
    </div>""")

# Quick Test Links
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Quick Tests</span>
            <span class="badge badge-cgi">demo</span>
        </div>
        <div class="btn-group">
            <a href="/cgi-bin/form.py" class="btn">Form Handler</a>
            <a href="/cgi-bin/api.py" class="btn">API Form</a>
            <a href="/cgi-bin/session.py" class="btn">Session Demo</a>
            <a href="/cgi-bin/cookies.py" class="btn">Cookie Demo</a>
            <a href="/cgi-bin/error.py" class="btn">Error Test</a>
            <a href="/cgi-bin/redirect.py" class="btn">Redirection CGI</a>
            <a href="/cgi-bin/sysinfo.sh" class="btn">Bash CGI</a>
            <a href="/cgi-bin/info.php" class="btn">PHP CGI</a>
            <a href="/cgi-bin/info.pl" class="btn">Perl CGI</a>
        </div>
    </div>""")

# Navigation
print("""
    <div class="card">
        <div class="card-header">
            <span class="card-title">Navigation</span>
        </div>
        <div class="btn-group">
            <a href="/cgi-bin/" class="btn">Test Center</a>
            <a href="/" class="btn">Home</a>
        </div>
    </div>""")

# JavaScript for filtering
print("""
    <script>
    function filterTable(tableId, filter) {
        var table = document.getElementById(tableId);
        var rows = table.getElementsByTagName('tr');
        filter = filter.toUpperCase();
        
        for (var i = 0; i < rows.length; i++) {
            var varCell = rows[i].getElementsByTagName('td')[0];
            if (varCell) {
                var varName = varCell.textContent || varCell.innerText;
                if (varName.toUpperCase().indexOf(filter) > -1) {
                    rows[i].style.display = '';
                } else {
                    rows[i].style.display = 'none';
                }
            }
        }
    }
    </script>""")

print(html.footer())