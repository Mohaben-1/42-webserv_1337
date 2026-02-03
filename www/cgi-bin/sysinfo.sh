#!/bin/bash

# Output HTTP headers
echo -e "Content-Type: text/html\r"
echo -e "\r"

# Generate HTML output
cat << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bash CGI Test</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
            min-height: 100vh;
            padding: 40px;
            color: #333;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        header {
            background: linear-gradient(135deg, #434343 0%, #000000 100%);
            padding: 30px;
            text-align: center;
            color: white;
        }
        header h1 { font-size: 2.5rem; margin-bottom: 10px; }
        header p { opacity: 0.9; font-family: monospace; }
        .content { padding: 30px; }
        .info-card {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .info-card h3 { color: #11998e; margin-bottom: 15px; }
        table { width: 100%; border-collapse: collapse; }
        td { padding: 8px 12px; border-bottom: 1px solid #dee2e6; }
        td:first-child { font-weight: 600; color: #495057; width: 40%; }
        .success { color: #28a745; font-weight: 600; }
        pre {
            background: #2d2d2d;
            color: #f8f8f2;
            padding: 15px;
            border-radius: 8px;
            overflow-x: auto;
            font-size: 0.9rem;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🐚 Bash CGI</h1>
            <p>#!/bin/bash</p>
        </header>
        <div class="content">
            <div class="info-card">
                <h3>System Information</h3>
                <table>
EOF

echo "                    <tr><td>Bash Version</td><td>${BASH_VERSION}</td></tr>"
echo "                    <tr><td>Hostname</td><td>$(hostname)</td></tr>"
echo "                    <tr><td>Current Time</td><td>$(date)</td></tr>"
echo "                    <tr><td>Uptime</td><td>$(uptime -p 2>/dev/null || echo 'N/A')</td></tr>"
echo "                    <tr><td>Kernel</td><td>$(uname -r)</td></tr>"
echo "                    <tr><td>Status</td><td class='success'>✓ Bash CGI Working!</td></tr>"

cat << 'EOF'
                </table>
            </div>
            <div class="info-card">
                <h3>Request Information</h3>
                <table>
EOF

echo "                    <tr><td>Request Method</td><td>${REQUEST_METHOD:-N/A}</td></tr>"
echo "                    <tr><td>Script Name</td><td>${SCRIPT_NAME:-N/A}</td></tr>"
echo "                    <tr><td>Query String</td><td>${QUERY_STRING:-N/A}</td></tr>"
echo "                    <tr><td>Server Software</td><td>${SERVER_SOFTWARE:-N/A}</td></tr>"
echo "                    <tr><td>Remote Address</td><td>${REMOTE_ADDR:-N/A}</td></tr>"

cat << 'EOF'
                </table>
            </div>
            <div class="info-card">
                <h3>Sample Command Output</h3>
                <pre>
EOF

echo "$ ls -la /tmp | head -5"
ls -la /tmp 2>/dev/null | head -5

cat << 'EOF'
                </pre>
            </div>
        </div>
    </div>
</body>
</html>
EOF
