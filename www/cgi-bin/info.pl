#!/usr/bin/perl
use strict;
use warnings;


# Get current date
my ($sec, $min, $hour, $mday, $mon, $year) = localtime(time);
$year += 1900;
$mon += 1;
my $current_date = sprintf("%04d-%02d-%02d %02d:%02d:%02d", 
    $year, $mon, $mday, $hour, $min, $sec);

# Get Perl version
my $perl_version = sprintf("%.3f", $]);

# Get query string
my %params;
if (defined $ENV{QUERY_STRING} && $ENV{QUERY_STRING} ne '') {
    my @pairs = split(/&/, $ENV{QUERY_STRING});
    foreach my $pair (@pairs) {
        my ($key, $value) = split(/=/, $pair, 2);
        $value = '' unless defined $value;
        $params{$key} = $value;
    }
}

# Start HTML output - Using qq{} to avoid @ interpolation
print qq{Content-Type: text/html; charset=utf-8\r\n\r\n};

print qq{<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Perl CGI - System Info</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #0a0c10;
            color: #f0f6fc;
            line-height: 1.6;
            padding: 40px 20px;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
        }
        .header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 40px;
            padding-bottom: 20px;
            border-bottom: 1px solid #30363d;
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
            background: linear-gradient(45deg, #f34f6d, #ff8c5a);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .badge {
            background: #161b22;
            padding: 8px 16px;
            border-radius: 20px;
            border: 1px solid #30363d;
            font-size: 14px;
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 16px;
            margin-bottom: 24px;
        }
        .stat-card {
            background: #161b22;
            border-radius: 12px;
            padding: 20px;
            border: 1px solid #30363d;
            box-shadow: 0 8px 24px rgba(0,0,0,0.2);
        }
        .stat-title {
            color: #8b949e;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 8px;
        }
        .stat-number {
            font-size: 24px;
            font-weight: 600;
            color: #f34f6d;
            font-family: monospace;
        }
        .card {
            background: #161b22;
            border-radius: 12px;
            padding: 24px;
            margin-bottom: 24px;
            border: 1px solid #30363d;
            box-shadow: 0 8px 24px rgba(0,0,0,0.2);
        }
        .card-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 1px solid #30363d;
            flex-wrap: wrap;
            gap: 12px;
        }
        .card-title {
            font-size: 18px;
            font-weight: 600;
            color: #f0f6fc;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .info-table {
            width: 100%;
            border-collapse: collapse;
        }
        .info-table tr {
            border-bottom: 1px solid #30363d;
        }
        .info-table tr:last-child {
            border-bottom: none;
        }
        .info-table td {
            padding: 12px 8px;
        }
        .info-table td:first-child {
            color: #8b949e;
            font-weight: 500;
            width: 30%;
            font-size: 14px;
            font-family: monospace;
        }
        .info-table td:last-child {
            font-family: monospace;
            color: #f34f6d;
            word-break: break-word;
        }
        .info-table tr:hover td {
            background: #21262d;
        }
        .alert {
            padding: 16px 20px;
            border-radius: 8px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
        }
        .alert-success {
            background: rgba(46, 160, 67, 0.15);
            border: 1px solid #2ea043;
            color: #2ea043;
        }
        .alert-info {
            background: rgba(88, 166, 255, 0.15);
            border: 1px solid #58a6ff;
            color: #58a6ff;
        }
        .btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            padding: 10px 20px;
            background: #21262d;
            color: #f0f6fc;
            text-decoration: none;
            border-radius: 6px;
            border: 1px solid #30363d;
            font-size: 14px;
            font-weight: 500;
            transition: all 0.2s ease;
            margin-right: 12px;
            margin-bottom: 8px;
        }
        .btn:hover {
            background: #30363d;
            border-color: #8b949e;
        }
        .btn-primary {
            background: #f34f6d;
            border-color: #f34f6d;
            color: white;
        }
        .btn-primary:hover {
            background: #ff6b84;
            border-color: #ff6b84;
        }
        .btn-info {
            background: #58a6ff;
            border-color: #58a6ff;
            color: white;
        }
        .btn-info:hover {
            background: #6cb4ff;
            border-color: #6cb4ff;
        }
        .btn-group {
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
            margin-top: 16px;
        }
        .footer {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #30363d;
            text-align: center;
            color: #8b949e;
            font-size: 13px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">
                <span class="logo-icon"></span>
                <span class="logo-text">Perl CGI</span>
            </div>
            <div class="badge">
                <span style="color: #f34f6d; font-weight: 600;">Perl $perl_version</span> 
                <span style="color: #8b949e;">| CGI/1.1</span>
            </div>
        </div>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-title">Perl Version</div>
                <div class="stat-number">$perl_version</div>
            </div>
            <div class="stat-card">
                <div class="stat-title">Server</div>
                <div class="stat-number" style="font-size: 20px; color: #58a6ff;">
                    $ENV{SERVER_SOFTWARE}
                </div>
            </div>
            <div class="stat-card">
                <div class="stat-title">Status</div>
                <div class="stat-number" style="font-size: 20px; color: #2ea043;">✓ Active</div>
            </div>
        </div>
};

# Query String Alert
if ($ENV{QUERY_STRING}) {
    print qq{
        <div class="alert alert-info">
            <span style="font-size: 24px;">🔍</span>
            <div style="flex: 1">
                <strong>Query String:</strong> $ENV{QUERY_STRING}
            </div>
        </div>
    };
}

# System Information Card
print qq{
        <div class="card">
            <div class="card-header">
                <span class="card-title">System Information</span>
                <span style="background: #21262d; padding: 4px 12px; border-radius: 20px; font-size: 12px; color: #8b949e;">$^O</span>
            </div>
            <table class="info-table">
                <tr><td>Perl Version</td><td>$]</td></tr>
                <tr><td>OS Name</td><td>$^O</td></tr>
                <tr><td>Server Software</td><td>$ENV{SERVER_SOFTWARE}</td></tr>
                <tr><td>Server Protocol</td><td>$ENV{SERVER_PROTOCOL}</td></tr>
                <tr><td>Gateway Interface</td><td>$ENV{GATEWAY_INTERFACE}</td></tr>
                <tr><td>Document Root</td><td>$ENV{DOCUMENT_ROOT}</td></tr>
            </table>
        </div>

        <div class="card">
            <div class="card-header">
                <span class="card-title">Request Information</span>
                <span style="background: #21262d; padding: 4px 12px; border-radius: 20px; font-size: 12px; color: #8b949e;">CGI</span>
            </div>
            <table class="info-table">
                <tr><td>Request Method</td><td>$ENV{REQUEST_METHOD}</td></tr>
                <tr><td>Script Name</td><td>$ENV{SCRIPT_NAME}</td></tr>
                <tr><td>Script Filename</td><td>$ENV{SCRIPT_FILENAME}</td></tr>
                <tr><td>Server Time</td><td style="color: #e3b341;">$current_date</td></tr>
            </table>
        </div>
};



# Quick Tests and Navigation
print qq{

        <div class="card">
            <div class="card-header">
                <span class="card-title">⚙️ Navigation</span>
            </div>
            <div class="btn-group">
                <a href="/cgi-bin/info.php" class="btn">PHP CGI</a>
                <a href="/cgi-bin/sysinfo.sh" class="btn">Bash CGI</a>
                <a href="/cgi-bin/test.py" class="btn">Python CGI</a>
                <a href="/cgi-bin/" class="btn">Test Center</a>
                <a href="/" class="btn">Home</a>
            </div>
        </div>

        <div class="footer">
            <p>Perl CGI Script • System Information • $current_date</p>
            <p style="margin-top: 8px; font-size: 12px; color: #8b949e;">
                Perl $] • CGI/1.1 • Server: $ENV{SERVER_SOFTWARE}
            </p>
        </div>
    </div>
</body>
</html>
};