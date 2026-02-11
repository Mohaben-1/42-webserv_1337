#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html; charset=utf-8");

// Get current date for footer
$current_date = date('Y-m-d H:i:s');
$php_version = phpversion();
$php_version_short = substr($php_version, 0, strpos($php_version, '-') ?: strlen($php_version));
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI - System Info</title>
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
            background: linear-gradient(45deg, #58a6ff, #f778ba);
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
            background: var(--critical);
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
            color: var(--critical);
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
            color: var(--critical);
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
        
        .value-not-set {
            color: var(--text-secondary) !important;
            font-style: italic;
        }
        
        .alert {
            padding: 16px 20px;
            border-radius: 8px;
            margin-bottom: 24px;
            display: flex;
            align-items: center;
            gap: 12px;
            animation: slideIn 0.3s ease;
        }
        
        @keyframes slideIn {
            from {
                opacity: 0;
                transform: translateY(-10px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        .alert-success {
            background: rgba(46, 160, 67, 0.15);
            border: 1px solid var(--accent);
            color: var(--accent);
        }
        
        .alert-info {
            background: rgba(88, 166, 255, 0.15);
            border: 1px solid var(--info);
            color: var(--info);
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
            background: var(--critical);
            border-color: var(--critical);
            color: white;
        }
        
        .btn-primary:hover {
            background: #ff99cc;
            border-color: #ff99cc;
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
        
        .btn-group {
            display: flex;
            gap: 12px;
            flex-wrap: wrap;
            margin-top: 16px;
        }
        
        .badge {
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 500;
            display: inline-block;
        }
        
        .badge-php {
            background: rgba(247, 120, 186, 0.2);
            color: var(--critical);
            border: 1px solid var(--critical);
        }
        
        .badge-cgi {
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
                <span class="logo-text">PHP CGI</span>
            </div>
            <div class="language-badge">
                <span class="language-tag">PHP <?php echo $php_version_short; ?></span>
                <span>CGI/1.1</span>
            </div>
        </div>

        <!-- Stats Grid -->
        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-title">PHP Version</div>
                <div class="stat-number" style="color: var(--critical);"><?php echo $php_version_short; ?></div>
            </div>
            <div class="stat-card">
                <div class="stat-title">Server</div>
                <div class="stat-number" style="font-size: 20px; color: var(--info);">
                    <?php 
                        $software = $_SERVER['SERVER_SOFTWARE'] ?? 'Unknown';
                        echo substr($software, 0, 20) . (strlen($software) > 20 ? '...' : '');
                    ?>
                </div>
            </div>
            <div class="stat-card">
                <div class="stat-title">Status</div>
                <div class="stat-number" style="font-size: 20px; color: var(--accent);">✓ Active</div>
            </div>
        </div>
        <!-- Query String Alert -->
        <?php if (!empty($_SERVER['QUERY_STRING'])): ?>
        <div class="alert alert-info">
            <span style="font-size: 24px;">🔍</span>
            <div style="flex: 1">
                <strong>Query String:</strong> <?php echo htmlspecialchars($_SERVER['QUERY_STRING']); ?>
            </div>
        </div>
        <?php endif; ?>

        <!-- PHP Information Card -->
        <div class="card">
            <div class="card-header">
                <span class="card-title">PHP Information</span>
                <span class="badge badge-php"><?php echo php_sapi_name(); ?></span>
            </div>
            <table class="info-table">
                <tr>
                    <td>PHP Version</td>
                    <td><?php echo $php_version; ?></td>
                </tr>
                <tr>
                    <td>Server Software</td>
                    <td><?php echo $_SERVER['SERVER_SOFTWARE'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
                <tr>
                    <td>Server Protocol</td>
                    <td><?php echo $_SERVER['SERVER_PROTOCOL'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
                <tr>
                    <td>Gateway Interface</td>
                    <td><?php echo $_SERVER['GATEWAY_INTERFACE'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
                <tr>
                    <td>Document Root</td>
                    <td><?php echo $_SERVER['DOCUMENT_ROOT'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
            </table>
        </div>

        <!-- Request Information Card -->
        <div class="card">
            <div class="card-header">
                <span class="card-title">Request Information</span>
                <span class="badge badge-cgi">CGI</span>
            </div>
            <table class="info-table">
                <tr>
                    <td>Request Method</td>
                    <td><?php echo $_SERVER['REQUEST_METHOD'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
                <tr>
                    <td>Script Name</td>
                    <td><?php echo $_SERVER['SCRIPT_NAME'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
                <tr>
                    <td>Script Filename</td>
                    <td><?php echo $_SERVER['SCRIPT_FILENAME'] ?? '<span class="value-not-set">N/A</span>'; ?></td>
                </tr>
            </table>
        </div>

        <!-- GET Parameters Card -->
        <?php if (!empty($_GET)): ?>
        <div class="card">
            <div class="card-header">
                <span class="card-title">GET Parameters</span>
                <span class="badge badge-php"><?php echo count($_GET); ?> fields</span>
            </div>
            <table class="info-table">
                <?php foreach ($_GET as $key => $value): ?>
                <tr>
                    <td><?php echo htmlspecialchars($key); ?></td>
                    <td style="color: var(--info);"><?php echo htmlspecialchars($value); ?></td>
                </tr>
                <?php endforeach; ?>
            </table>
        </div>
        <?php endif; ?>

        <!-- HTTP Headers Card -->
        <div class="card">
            <div class="card-header">
                <span class="card-title">HTTP Headers</span>
                <span class="badge badge-cgi">request</span>
            </div>
            <table class="info-table">
                <?php
                $headers = [];
                foreach ($_SERVER as $key => $value) {
                    if (strpos($key, 'HTTP_') === 0) {
                        $header = str_replace('_', '-', substr($key, 5));
                        $header = ucwords(strtolower($header), '-');
                        $headers[$header] = $value;
                    }
                }
                ?>
                <?php if (!empty($headers)): ?>
                    <?php foreach ($headers as $header => $value): ?>
                    <tr>
                        <td><?php echo htmlspecialchars($header); ?></td>
                        <td style="color: var(--text-secondary);"><?php echo htmlspecialchars($value); ?></td>
                    </tr>
                    <?php endforeach; ?>
                <?php else: ?>
                <tr>
                    <td colspan="2" style="color: var(--text-secondary); text-align: center; padding: 20px;">
                        No HTTP headers received
                    </td>
                </tr>
                <?php endif; ?>
            </table>
        </div>

        <!-- Time Information Card -->
        <div class="card">
            <div class="card-header">
                <span class="card-title">Time Information</span>
                <span class="badge badge-php"><?php echo date_default_timezone_get(); ?></span>
            </div>
            <table class="info-table">
                <tr>
                    <td>Server Time</td>
                    <td style="color: var(--warning);"><?php echo date('Y-m-d H:i:s'); ?></td>
                </tr>
                <tr>
                    <td>Timezone</td>
                    <td><?php echo date_default_timezone_get(); ?></td>
                </tr>
                <tr>
                    <td>Unix Timestamp</td>
                    <td><?php echo time(); ?></td>
                </tr>
            </table>
        </div>


        <!-- Navigation -->
        <div class="card">
            <div class="card-header">
                <span class="card-title">Navigation</span>
            </div>
            <div class="btn-group">
                <a href="/cgi-bin/bash.sh" class="btn">Bash CGI</a>
                <a href="/cgi-bin/info.pl" class="btn">Perl CGI</a>
                <a href="/cgi-bin/test.py" class="btn">Python CGI</a>
                <a href="/cgi-bin/" class="btn">Test Center</a>
                <a href="/" class="btn">Home</a>
            </div>
        </div>

        <!-- Footer -->
        <div class="footer">
            <p>PHP CGI Script • System Information • <?php echo $current_date; ?></p>
            <p style="margin-top: 8px; font-size: 12px; color: var(--text-secondary);">
                PHP <?php echo $php_version; ?> • CGI/1.1 • Server: <?php echo $_SERVER['SERVER_SOFTWARE'] ?? 'Unknown'; ?>
            </p>
        </div>
    </div>
</body>
</html>
