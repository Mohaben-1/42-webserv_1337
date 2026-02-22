#!/bin/bash

HOST=${1:-127.0.0.1}
PORT=${2:-8080}
BASE="http://$HOST:$PORT"

echo "========================================"
echo "WEB SERV AUTOMATED TEST SUITE"
echo "Target: $BASE"
echo "========================================"

pass() { echo -e "[PASS] $1"; }
fail() { echo -e "[FAIL] $1"; }

check_status() {
    EXPECT=$1
    URL=$2
    METHOD=${3:-GET}

    STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X $METHOD "$URL")

    if [ "$STATUS" = "$EXPECT" ]; then
        pass "$METHOD $URL returned $STATUS"
    else
        fail "$METHOD $URL returned $STATUS (expected $EXPECT)"
    fi
}

echo
echo "===== BASIC STATUS CODE TESTS ====="

check_status 200 "$BASE/"
check_status 404 "$BASE/does_not_exist"
check_status 405 "$BASE/" DELETE

echo
echo "===== GET FILE TEST ====="

echo "test" > testfile.txt
curl -s "$BASE/files/file1.txt" | grep -q "test" && pass "GET file works" || fail "GET file failed"

echo
echo "===== POST UPLOAD TEST ====="

UPLOAD_RESULT=$(curl -s -X POST "$BASE/upload" -d "HELLO123")
echo "$UPLOAD_RESULT" | grep -q "success" && pass "POST upload works" || fail "POST upload failed"

echo
echo "===== DELETE TEST ====="

FILENAME=$(echo "$UPLOAD_RESULT" | grep -o 'upload_[^"]*')
if [ -n "$FILENAME" ]; then
    DELETE_RESULT=$(curl -s -X DELETE "$BASE/uploads/$FILENAME")
    echo "$DELETE_RESULT" | grep -q "success" && pass "DELETE works" || fail "DELETE failed"
else
    fail "Could not extract filename"
fi

echo
echo "===== CLIENT BODY LIMIT TEST ====="

head -c 100 /dev/zero | curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/upload" --data-binary @- > small.txt
head -c 20000000 /dev/zero | curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/upload" --data-binary @- > big.txt

SMALL=$(cat small.txt)
BIG=$(cat big.txt)

echo "Small body status: $SMALL"
echo "Big body status: $BIG"

if [ "$BIG" = "413" ]; then
    pass "Body limit enforced"
else
    fail "Body limit NOT enforced"
fi

echo
echo "===== INVALID METHOD TEST ====="

STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X FAKE "$BASE/")
[ "$STATUS" = "400" ] || [ "$STATUS" = "405" ] \
    && pass "Invalid method handled" \
    || fail "Invalid method not handled properly"

echo
echo "===== DIRECTORY DEFAULT FILE TEST ====="

STATUS=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/")
[ "$STATUS" = "200" ] \
    && pass "Default index works" \
    || fail "Default index missing"

echo
echo "===== MULTIPLE PORT TEST ====="

for P in 8080 8081 8082
do
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://$HOST:$P/)
    echo "Port $P -> $STATUS"
done

echo
echo "===== HOSTNAME TEST ====="

STATUS=$(curl -s -o /dev/null -w "%{http_code}" \
--resolve example.com:$PORT:$HOST \
http://example.com:$PORT/)

[ "$STATUS" = "200" ] \
    && pass "Hostname routing works" \
    || fail "Hostname routing failed"

echo
echo "===== STRESS TEST (SIEGE) ====="

if command -v siege >/dev/null 2>&1; then
    siege -b -t10s -c50 "$BASE/" > siege.txt

    AVAIL=$(grep "Availability" siege.txt | awk '{print $2}' | sed 's/%//')

    echo "Availability: $AVAIL%"

    AVAIL_INT=${AVAIL%.*}

    if [ "$AVAIL_INT" -ge 99 ]; then
        pass "Stress test passed"
    else
        fail "Stress test failed"
    fi
else
    echo "Siege not installed, skipping stress test"
fi

echo
echo "===== HANGING CONNECTION TEST ====="

BEFORE=$(lsof -i :$PORT | wc -l)

curl -s "$BASE/" > /dev/null

sleep 1

AFTER=$(lsof -i :$PORT | wc -l)

if [ "$AFTER" -le "$BEFORE" ]; then
    pass "No hanging connections"
else
    fail "Connections hanging"
fi

echo
echo "===== MEMORY TEST ====="

PID=$(pgrep webserv)

if [ -n "$PID" ]; then
    ps -o rss= -p $PID
    pass "Memory measurable"
else
    fail "Cannot find webserv process"
fi

echo
echo "========================================"
echo "TEST COMPLETE"
echo "========================================"
