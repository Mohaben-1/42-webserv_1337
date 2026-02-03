#!/usr/bin/env python3
"""
CGI script with infinite loop - for testing timeout handling
Server should kill this after CGI_TIMEOUT seconds
"""
import sys
import time

print("Content-Type: text/html")
print("")
print("""<!DOCTYPE html>
<html>
<head>
    <title>Timeout Test</title>
    <style>
        body { font-family: sans-serif; padding: 40px; background: #1a1a2e; color: #fff; }
        .box { background: #16213e; padding: 30px; border-radius: 10px; max-width: 500px; }
        h1 { color: #e94560; }
        .spin { display: inline-block; animation: spin 1s linear infinite; }
        @keyframes spin { 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div class="box">
        <h1><span class="spin">⏳</span> Infinite Loop Test</h1>
        <p>This script loops forever until the server timeout kills it.</p>
        <p>If you see this, the server should terminate this script soon...</p>
    </div>
</body>
</html>""")
sys.stdout.flush()

while True:
    time.sleep(1)
