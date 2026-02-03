#!/usr/bin/perl
use strict;
use warnings;

# Output HTTP headers
print "Content-Type: text/html\r\n";
print "\r\n";

# Generate HTML output
print <<'HTML';
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Perl CGI Test</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
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
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
            padding: 30px;
            text-align: center;
            color: white;
        }
        header h1 { font-size: 2.5rem; margin-bottom: 10px; }
        header p { opacity: 0.9; }
        .content { padding: 30px; }
        .info-card {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .info-card h3 { color: #764ba2; margin-bottom: 15px; }
        table { width: 100%; border-collapse: collapse; }
        td { padding: 8px 12px; border-bottom: 1px solid #dee2e6; }
        td:first-child { font-weight: 600; color: #495057; width: 40%; }
        .success { color: #28a745; font-weight: 600; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🐪 Perl CGI</h1>
            <p>Multi-CGI Support Demonstration</p>
        </header>
        <div class="content">
            <div class="info-card">
                <h3>Script Information</h3>
                <table>
HTML

print "                    <tr><td>Perl Version</td><td>$^V</td></tr>\n";
print "                    <tr><td>Script Name</td><td>$ENV{SCRIPT_NAME} // 'N/A'</td></tr>\n";
print "                    <tr><td>Request Method</td><td>$ENV{REQUEST_METHOD} // 'N/A'</td></tr>\n";
print "                    <tr><td>Server Software</td><td>$ENV{SERVER_SOFTWARE} // 'N/A'</td></tr>\n";
print "                    <tr><td>Status</td><td class='success'>✓ Perl CGI Working!</td></tr>\n";

print <<'HTML';
                </table>
            </div>
            <div class="info-card">
                <h3>Environment Variables</h3>
                <table>
HTML

foreach my $key (sort keys %ENV) {
    my $value = $ENV{$key};
    $value = substr($value, 0, 50) . "..." if length($value) > 50;
    print "                    <tr><td>$key</td><td>$value</td></tr>\n";
}

print <<'HTML';
                </table>
            </div>
        </div>
    </div>
</body>
</html>
HTML
