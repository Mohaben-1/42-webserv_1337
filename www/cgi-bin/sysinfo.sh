#!/bin/bash

# Output HTTP headers
echo -e "Content-Type: text/html; charset=utf-8\r"
echo -e "\r"

# Get current date for footer
CURRENT_DATE=$(date '+%Y-%m-%d %H:%M:%S')

# Generate HTML output
cat << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bash CGI - System Info</title>
    <style>
        :root {
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
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: var(--bg-primary);
            color: var(--text-primary);
            line-height: 1.6;
            min-height: 100vh;
            padding: 40px 20px;
        }
        
        .container {
            max-width: 900px;
            margin: 0 auto;
        }
        
        .header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 40px;
            padding-bottom: 20px;
            border-bottom: 1px solid var(--border);
            flex-wrap: wrap;
            gap: 16px;
        }
        
        .logo {
            display: flex;
            align-items: center;
            gap: 12px;
        }
        
        .logo-icon {
            font-size: 32px;
        }
        
        .logo-text {
            font-size: 24px;
            font-weight: 600;
            background: linear-gradient(45deg, #2ea043, #58a6ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        
        .language-badge {
            background: var(--bg-secondary);
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 14px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .language-tag {
            background: var(--accent);
            color: white;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
        }
        
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }
        
        .stat-card {
            background: var(--bg-secondary);
            border-radius: 12px;
            padding: 20px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow);
        }
        
        .stat-title {
            color: var(--text-secondary);
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 8px;
            display: flex;
            align-items: center;
            gap: 6px;
        }
        
        .stat-number {
            font-size: 24px;
            font-weight: 600;
            color: var(--accent);
            font-family: monospace;
        }
        
        .card {
            background: var(--bg-secondary);
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 24px;
            border: 1px solid var(--border);
            box-shadow: var(--shadow);
        }
        
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid var(--border);
            flex-wrap: wrap;
            gap: 12px;
        }
        
        .card-title {
            font-size: 18px;
            font-weight: 600;
            color: var(--text-primary);
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .info-table {
            width: 100%;
            border-collapse: collapse;
        }
        
        .info-table tr {
            border-bottom: 1px solid var(--border);
        }
        
        .info-table tr:last-child {
            border-bottom: none;
        }
        
        .info-table td {
            padding: 12px 8px;
            color: var(--text-primary);
        }
        
        .info-table td:first-child {
            color: var(--text-secondary);
            font-weight: 500;
            width: 40%;
            font-size: 14px;
        }
        
        .info-table td:last-child {
            font-family: 'SF Mono', Monaco, 'Cascadia Code', 'Roboto Mono', monospace;
            color: var(--info);
        }
        
        .success-badge {
            display: inline-block;
            padding: 4px 12px;
            background: rgba(46, 160, 67, 0.15);
            color: var(--accent);
            border: 1px solid var(--accent);
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
        }
        
        .code-block {
            background: var(--bg-primary);
            border: 1px solid var(--border);
            border-radius: 8px;
            padding: 16px;
            font-family: 'SF Mono', Monaco, 'Cascadia Code', 'Roboto Mono', monospace;
            font-size: 13px;
            line-height: 1.5;
            color: var(--text-secondary);
            overflow-x: auto;
            margin-top: 8px;
        }
        
        .code-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
            color: var(--text-secondary);
            font-size: 12px;
        }
        
        .code-prompt {
            color: var(--accent);
        }
        
        .code-output {
            color: var(--text-primary);
        }
        
        .btn {
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
        }
        
        .btn:hover {
            background: var(--border);
            border-color: var(--text-secondary);
        }
        
        .btn-primary {
            background: var(--accent);
            border-color: var(--accent);
            color: white;
        }
        
        .btn-primary:hover {
            background: var(--accent-hover);
            border-color: var(--accent-hover);
        }
        
        .btn-info {
            background: var(--info);
            border-color: var(--info);
            color: white;
        }
        
        .btn-info:hover {
            background: #6cb4ff;
            border-color: #6cb4ff;
        }
        
        .badge {
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
            display: inline-block;
        }
        
        .badge-bash {
            background: rgba(46, 160, 67, 0.2);
            color: var(--accent);
            border: 1px solid var(--accent);
        }
        
        .footer {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid var(--border);
            text-align: center;
            color: var(--text-secondary);
            font-size: 13px;
        }
        
        @media (max-width: 600px) {
            .header {
                flex-direction: column;
                align-items: start;
            }
            
            .stats-grid {
                grid-template-columns: 1fr;
            }
            
            .btn {
                width: 100%;
                margin-right: 0;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon"></span>
                <span class="logo-text">Bash CGI</span>
            </div>
EOF

# Language badge
echo "            <div class=\"language-badge\">"
echo "                <span class=\"language-tag\">BASH</span>"
echo "                <span>${BASH_VERSION%%(*}</span>"
echo "            </div>"
echo "        </div>"

# Stats grid
echo "        <div class=\"stats-grid\">"
echo "            <div class=\"stat-card\">"
echo "                <div class=\"stat-title\">Hostname</div>"
echo "                <div class=\"stat-number\" style=\"color: var(--info);\">$(hostname | cut -d. -f1)</div>"
echo "            </div>"
echo "            <div class=\"stat-card\">"
echo "                <div class=\"stat-title\">Kernel</div>"
echo "                <div class=\"stat-number\" style=\"font-size: 20px; color: var(--warning);\">$(uname -r | cut -d- -f1)</div>"
echo "            </div>"
echo "            <div class=\"stat-card\">"
echo "                <div class=\"stat-title\">Status</div>"
echo "                <div class=\"stat-number\" style=\"font-size: 20px; color: var(--accent);\">✓ Active</div>"
echo "            </div>"
echo "        </div>"

# System Information Card
echo "        <div class=\"card\">"
echo "            <div class=\"card-header\">"
echo "                <span class=\"card-title\">System Information</span>"
echo "                <span class=\"badge badge-bash\">$(uname -s)</span>"
echo "            </div>"
echo "            <table class=\"info-table\">"

# System info rows
echo "                <tr><td>Bash Version</td><td>${BASH_VERSION}</td></tr>"
echo "                <tr><td>Hostname</td><td>$(hostname)</td></tr>"
echo "                <tr><td>Current Time</td><td>$(date '+%Y-%m-%d %H:%M:%S')</td></tr>"
echo "                <tr><td>Uptime</td><td>$(uptime -p 2>/dev/null | sed 's/up //' || echo 'N/A')</td></tr>"
echo "                <tr><td>Kernel Release</td><td>$(uname -r)</td></tr>"
echo "                <tr><td>Architecture</td><td>$(uname -m)</td></tr>"
echo "                <tr><td>Operating System</td><td>$(uname -s)</td></tr>"

echo "            </table>"
echo "        </div>"

# Request Information Card
echo "        <div class=\"card\">"
echo "            <div class=\"card-header\">"
echo "                <span class=\"card-title\">Request Information</span>"
echo "                <span class=\"badge badge-bash\">CGI/1.1</span>"
echo "            </div>"
echo "            <table class=\"info-table\">"

# Request info rows
echo "                <tr><td>Request Method</td><td>${REQUEST_METHOD:-N/A}</td></tr>"
echo "                <tr><td>Script Name</td><td>${SCRIPT_NAME:-N/A}</td></tr>"
echo "                <tr><td>Script Filename</td><td>${SCRIPT_FILENAME:-N/A}</td></tr>"
echo "                <tr><td>Query String</td><td>${QUERY_STRING:-N/A}</td></tr>"
echo "                <tr><td>Server Software</td><td>${SERVER_SOFTWARE:-N/A}</td></tr>"
echo "                <tr><td>Server Protocol</td><td>${SERVER_PROTOCOL:-N/A}</td></tr>"
echo "                <tr><td>Gateway Interface</td><td>${GATEWAY_INTERFACE:-N/A}</td></tr>"

echo "            </table>"
echo "        </div>"

# Environment Variables Card (sample)
echo "        <div class=\"card\">"
echo "            <div class=\"card-header\">"
echo "                <span class=\"card-title\">Environment</span>"
echo "                <span class=\"badge badge-bash\">selected</span>"
echo "            </div>"
echo "            <table class=\"info-table\">"
echo "                <tr><td>USER</td><td>${USER:-N/A}</td></tr>"
echo "                <tr><td>HOME</td><td>${HOME:-N/A}</td></tr>"
echo "                <tr><td>PWD</td><td>${PWD:-N/A}</td></tr>"
echo "                <tr><td>SHELL</td><td>${SHELL:-N/A}</td></tr>"
echo "                <tr><td>PATH</td><td style=\"font-size: 11px;\">${PATH:0:60}...</td></tr>"
echo "            </table>"
echo "        </div>"

# Command Output Card
echo "        <div class=\"card\">"
echo "            <div class=\"card-header\">"
echo "                <span class=\"card-title\">Command Output</span>"
echo "                <span class=\"badge badge-bash\">/tmp listing</span>"
echo "            </div>"
echo "            <div class=\"code-header\">"
echo "                <span class=\"code-prompt\">$ ls -la /tmp | head -5</span>"
echo "            </div>"
echo "            <div class=\"code-block\">"
ls -la /tmp 2>/dev/null | head -5 | while read line; do
    echo "                <span class=\"code-output\">${line}</span><br>"
done
echo "            </div>"
echo "        </div>"

# Navigation Card
echo "        <div class=\"card\">"
echo "            <div class=\"card-header\">"
echo "                <span class=\"card-title\">Navigation</span>"
echo "            </div>"
echo "            <div style=\"display: flex; gap: 12px; flex-wrap: wrap;\">"
echo "                <a href=\"/cgi-bin/test.py\" class=\"btn\">Python CGI</a>"
echo "                <a href=\"/cgi-bin/info.pl\" class=\"btn\">Perl CGI</a>"
echo "                <a href=\"/cgi-bin/info.php\" class=\"btn\">PHP CGI</a>"
echo "                <a href=\"/cgi-bin/\" class=\"btn\">Test Center</a>"
echo "                <a href=\"/\" class=\"btn\">Home</a>"
echo "            </div>"
echo "        </div>"

# Footer
cat << EOF
        <div class="footer">
            <p>Bash CGI Script • System Information • ${CURRENT_DATE}</p>
            <p style="margin-top: 8px; font-size: 12px; color: var(--text-secondary);">#!/bin/bash • CGI/1.1 • Server: ${SERVER_SOFTWARE:-Unknown}</p>
        </div>
    </div>
</body>
</html>
EOF