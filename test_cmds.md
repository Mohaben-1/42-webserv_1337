# Webserv Testing Guide

This document contains all the tests required to validate the Webserv project according to the subject and evaluation sheet.

---

# 1. Compilation Test

Ensure project compiles correctly:

```bash
make
make clean
make
make fclean
make
```

Expected result:

• No errors
• No unnecessary relinking
• Compiles with:

```bash
c++ -Wall -Wextra -Werror -std=c++98
```

---

# 2. Server Launch Test

Start the server:

```bash
./webserv config.conf
```

Expected result:

• Server starts
• No crash
• Correct ports open

Verify:

```bash
lsof -i -P -n | grep webserv
```

---

# 3. Basic HTTP Tests

## GET request

```bash
curl -v http://127.0.0.1:8080/
```

Expected:

```
HTTP/1.1 200 OK
```

---

## GET non-existing file

```bash
curl -v http://127.0.0.1:8080/does_not_exist.html
```

Expected:

```
HTTP/1.1 404 Not Found
```

---

## POST request

```bash
curl -v -X POST http://127.0.0.1:8080/upload -d "hello"
```

Expected:

```
HTTP/1.1 201 Created
```

File must be created in upload directory.

---

## DELETE request

```bash
curl -v -X DELETE http://127.0.0.1:8080/uploads/file
```

Expected:

```
HTTP/1.1 200 OK
or
HTTP/1.1 204 No Content
```

---

## Invalid method

```bash
curl -v -X FAKE http://127.0.0.1:8080/
```

Expected:

```
HTTP/1.1 400 Bad Request
or
HTTP/1.1 405 Method Not Allowed
```

Server must NOT crash.

---

# 4. Default Error Page Test

Verify custom error page:

```bash
curl http://127.0.0.1:8080/does_not_exist
```

Expected:

Custom HTML error page.

---

# 5. Client Body Size Limit Test

Small body:

```bash
head -c 100 /dev/zero | curl -v -X POST http://127.0.0.1:8080/upload --data-binary @-
```

Expected:

```
201 Created
```

Large body (must fail):

```bash
head -c 20000000 /dev/zero | curl -v -X POST http://127.0.0.1:8080/upload --data-binary @-
```

Expected:

```
413 Payload Too Large
```

---

# 6. Directory Default File Test

```bash
curl http://127.0.0.1:8080/
```

Expected:

index.html is returned.

---

# 7. Directory Listing Test

If enabled:

```bash
curl http://127.0.0.1:8080/uploads/
```

Expected:

Directory listing or index file.

---

# 8. Multiple Ports Test

Config example:

```
server {
    listen 8080;
}

server {
    listen 8081;
}
```

Test:

```bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8081/
```

Expected:

Both work.

---

# 9. Hostname Virtual Server Test

```bash
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
```

Expected:

Correct website returned.

---

# 10. File Upload Test

Upload file:

```bash
curl -X POST http://127.0.0.1:8080/upload -d "test upload"
```

Verify file exists:

```bash
ls uploads/
```

Download file:

```bash
curl http://127.0.0.1:8080/uploads/filename
```

---

# 11. Stress Test (Siege)

Install siege:

```bash
brew install siege
```

Run test:

```bash
siege -b -t60s -c100 http://127.0.0.1:8080/
```

Expected:

Availability ≥ 99.5%

---

# 12. Hanging Connection Test

Run siege:

```bash
siege -b http://127.0.0.1:8080/
```

Check connections:

```bash
lsof -i :8080
```

Expected:

No accumulation of CLOSE_WAIT connections.

---

# 13. Memory Leak Test

Find PID:

```bash
pgrep webserv
```

Monitor memory:

```bash
top -pid <PID>
```

Expected:

Memory usage stable.

---

# 14. Telnet Raw HTTP Test

```bash
telnet 127.0.0.1 8080
```

Send:

```
GET / HTTP/1.1
Host: localhost

```

Expected:

Valid HTTP response.

---

# 15. Browser Test

Open browser:

```
http://127.0.0.1:8080/
```

Verify:

• Page loads
• No errors
• Static files served

Open developer tools → Network tab.

Verify headers.

---

# 16. CGI Test

GET CGI:

```bash
curl http://127.0.0.1:8080/script.py
```

POST CGI:

```bash
curl -X POST http://127.0.0.1:8080/script.py -d "data"
```

Expected:

CGI executes correctly.

---

# 17. Server Resilience Test

Run indefinitely:

```bash
siege -b http://127.0.0.1:8080/
```

Expected:

Server never crashes.

---

# 18. Configuration Error Test

Invalid config:

```bash
./webserv invalid.conf
```

Expected:

Error message
No crash

---

# Success Criteria

Server must:

• Never crash
• Handle GET POST DELETE
• Enforce body size limits
• Serve static files
• Handle multiple ports
• Handle errors correctly
• Pass stress tests
• Not leak memory
• Not hang connections

---

# Conclusion

If all tests pass, your Webserv implementation is compliant with subject and evaluation requirements.

---
