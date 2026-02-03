#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Test</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 40px;
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
            background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
            padding: 30px;
            text-align: center;
            color: white;
        }
        header h1 { font-size: 2.5rem; margin-bottom: 10px; }
        .content { padding: 30px; }
        .info-card {
            background: #f8f9fa;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .info-card h3 { color: #667eea; margin-bottom: 15px; }
        table { width: 100%; border-collapse: collapse; }
        td { padding: 8px 12px; border-bottom: 1px solid #dee2e6; }
        td:first-child { font-weight: 600; color: #495057; width: 40%; }
        .success { color: #28a745; font-weight: 600; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>🐘 PHP CGI</h1>
            <p>Multi-CGI Support Demonstration</p>
        </header>
        <div class="content">
            <div class="info-card">
                <h3>PHP Information</h3>
                <table>
                    <tr><td>PHP Version</td><td><?php echo phpversion(); ?></td></tr>
                    <tr><td>Server Software</td><td><?php echo $_SERVER['SERVER_SOFTWARE'] ?? 'N/A'; ?></td></tr>
                    <tr><td>Request Method</td><td><?php echo $_SERVER['REQUEST_METHOD'] ?? 'N/A'; ?></td></tr>
                    <tr><td>Script Name</td><td><?php echo $_SERVER['SCRIPT_NAME'] ?? 'N/A'; ?></td></tr>
                    <tr><td>Query String</td><td><?php echo $_SERVER['QUERY_STRING'] ?? 'N/A'; ?></td></tr>
                    <tr><td>Status</td><td class="success">✓ PHP CGI Working!</td></tr>
                </table>
            </div>
            <div class="info-card">
                <h3>Current Time</h3>
                <table>
                    <tr><td>Server Time</td><td><?php echo date('Y-m-d H:i:s'); ?></td></tr>
                    <tr><td>Timezone</td><td><?php echo date_default_timezone_get(); ?></td></tr>
                </table>
            </div>
            <?php if (!empty($_GET)): ?>
            <div class="info-card">
                <h3>GET Parameters</h3>
                <table>
                    <?php foreach ($_GET as $key => $value): ?>
                    <tr><td><?php echo htmlspecialchars($key); ?></td><td><?php echo htmlspecialchars($value); ?></td></tr>
                    <?php endforeach; ?>
                </table>
            </div>
            <?php endif; ?>
        </div>
    </div>
</body>
</html>
