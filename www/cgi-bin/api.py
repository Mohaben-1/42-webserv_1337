#!/usr/bin/env python3
"""
CGI script that outputs JSON
"""
import os
import sys
import json
import datetime

# Output headers for JSON
print("Content-Type: application/json")
print("")

# Build response data
data = {
    "status": "success",
    "message": "CGI JSON endpoint working",
    "timestamp": datetime.datetime.now().isoformat(),
    "request": {
        "method": os.environ.get('REQUEST_METHOD', 'unknown'),
        "query_string": os.environ.get('QUERY_STRING', ''),
        "content_type": os.environ.get('CONTENT_TYPE', ''),
        "content_length": os.environ.get('CONTENT_LENGTH', '0'),
        "script_name": os.environ.get('SCRIPT_NAME', ''),
        "path_info": os.environ.get('PATH_INFO', ''),
    },
    "server": {
        "name": os.environ.get('SERVER_NAME', 'unknown'),
        "port": os.environ.get('SERVER_PORT', 'unknown'),
        "software": os.environ.get('SERVER_SOFTWARE', 'unknown'),
    }
}

# If POST, try to read body
method = os.environ.get('REQUEST_METHOD', 'GET')
if method == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        data["request"]["body"] = post_data[:1000]  # Limit size
        data["request"]["body_length"] = len(post_data)

# Output JSON
print(json.dumps(data, indent=2))
