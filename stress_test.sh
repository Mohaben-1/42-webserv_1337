#!/bin/bash

# Simple stress test script for webserv
# Alternative to siege when siege is not available

HOST="localhost"
PORT="8080"
URL="http://${HOST}:${PORT}/index.html"
DURATION=60
CONCURRENT=50
TOTAL_REQUESTS=0
FAILED_REQUESTS=0

echo "============================================"
echo "Webserv Stress Test"
echo "============================================"
echo "URL: $URL"
echo "Duration: ${DURATION}s"
echo "Concurrent connections: $CONCURRENT"
echo "============================================"

# Check if server is running
if ! curl -s -o /dev/null -w "%{http_code}" "$URL" | grep -q "200"; then
    echo "ERROR: Server is not responding at $URL"
    echo "Start the server first: ./webserv config/default.conf"
    exit 1
fi

# Get initial memory usage
WEBSERV_PID=$(pgrep -f "./webserv")
if [ -z "$WEBSERV_PID" ]; then
    echo "WARNING: Could not find webserv process for memory monitoring"
else
    INITIAL_MEM=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null)
    echo "Initial memory usage: ${INITIAL_MEM} KB"
fi

echo ""
echo "Starting stress test..."
echo ""

START_TIME=$(date +%s)
END_TIME=$((START_TIME + DURATION))

# Function to make requests
make_requests() {
    local count=0
    local failed=0
    while [ $(date +%s) -lt $END_TIME ]; do
        response=$(curl -s -o /dev/null -w "%{http_code}" "$URL" 2>/dev/null)
        count=$((count + 1))
        if [ "$response" != "200" ]; then
            failed=$((failed + 1))
        fi
    done
    echo "$count $failed"
}

# Run concurrent workers
PIDS=""
TMPDIR="/tmp/stress_test_$$"
mkdir -p $TMPDIR

for i in $(seq 1 $CONCURRENT); do
    (
        result=$(make_requests)
        echo "$result" > "$TMPDIR/worker_$i"
    ) &
    PIDS="$PIDS $!"
done

# Wait for all workers with progress indicator
echo -n "Running"
while [ $(date +%s) -lt $END_TIME ]; do
    echo -n "."
    sleep 5
done
echo " Done!"

# Wait for workers to finish
wait $PIDS 2>/dev/null

# Collect results
for i in $(seq 1 $CONCURRENT); do
    if [ -f "$TMPDIR/worker_$i" ]; then
        result=$(cat "$TMPDIR/worker_$i")
        count=$(echo $result | cut -d' ' -f1)
        failed=$(echo $result | cut -d' ' -f2)
        TOTAL_REQUESTS=$((TOTAL_REQUESTS + count))
        FAILED_REQUESTS=$((FAILED_REQUESTS + failed))
    fi
done

rm -rf $TMPDIR

# Calculate results
SUCCESS_REQUESTS=$((TOTAL_REQUESTS - FAILED_REQUESTS))
if [ $TOTAL_REQUESTS -gt 0 ]; then
    AVAILABILITY=$(awk "BEGIN {printf \"%.2f\", ($SUCCESS_REQUESTS * 100) / $TOTAL_REQUESTS}")
else
    AVAILABILITY="0"
fi

# Get final memory usage
if [ -n "$WEBSERV_PID" ]; then
    FINAL_MEM=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null)
    MEM_DIFF=$((FINAL_MEM - INITIAL_MEM))
fi

echo ""
echo "============================================"
echo "RESULTS"
echo "============================================"
echo "Total requests:     $TOTAL_REQUESTS"
echo "Successful:         $SUCCESS_REQUESTS"
echo "Failed:             $FAILED_REQUESTS"
echo "Availability:       ${AVAILABILITY}%"
echo ""

if [ -n "$WEBSERV_PID" ]; then
    echo "Memory (initial):   ${INITIAL_MEM} KB"
    echo "Memory (final):     ${FINAL_MEM} KB"
    echo "Memory difference:  ${MEM_DIFF} KB"
fi

echo "============================================"

# Check requirements
echo ""
echo "EVALUATION:"

AVAIL_PASS=$(awk "BEGIN {print ($AVAILABILITY >= 99.5) ? 1 : 0}")
if [ "$AVAIL_PASS" -eq 1 ]; then
    echo "✅ Availability >= 99.5%: PASSED ($AVAILABILITY%)"
else
    echo "❌ Availability >= 99.5%: FAILED ($AVAILABILITY%)"
fi

if [ -n "$MEM_DIFF" ]; then
    if [ $MEM_DIFF -lt 5000 ]; then
        echo "✅ No significant memory leak: PASSED (+${MEM_DIFF} KB)"
    else
        echo "⚠️  Memory increased significantly: CHECK (+${MEM_DIFF} KB)"
    fi
fi

# Check for hanging connections
FD_COUNT=$(ls /proc/$WEBSERV_PID/fd 2>/dev/null | wc -l)
echo "Open file descriptors: $FD_COUNT"

echo ""
echo "============================================"
