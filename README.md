*This project has been created as part of the 42 curriculum by mohaben-, ahouass, jhamdaou.*

# Webserv

## Description

Webserv is a custom HTTP server written in C++98, developed as part of the 42 curriculum.  
The goal of this project is to understand how web servers work internally by building one from scratch.

The server handles HTTP requests, serves static files, supports multiple virtual servers, manages client connections using non-blocking I/O, and can execute CGI scripts (such as PHP or Python).  

This project explores low-level networking concepts including:
- TCP/IP socket programming
- Multiplexing with `select()`, `poll()`, or `epoll()`
- HTTP protocol parsing
- Process management for CGI
- Configuration file parsing

The objective is to reproduce key behaviors of servers such as Nginx while respecting the constraints of C++98.

---

## Features

- HTTP/1.1 compliant request parsing
- Support for GET, POST, and DELETE methods
- Static file serving
- Directory listing
- File upload handling
- CGI execution (PHP, Python, etc.)
- Custom error pages
- Multiple server blocks (virtual hosting)
- Non-blocking I/O with a single multiplexing loop
- Configuration file similar to Nginx format

---

## Instructions

### Compilation

Clone the repository and run:

```bash
make
````

This will generate the executable:

```
./webserv [config_file]
```

To clean object files:

```bash
make clean
```

To remove everything including the executable:

```bash
make fclean
```

To recompile:

```bash
make re
```

---

### Execution

Run the server with a configuration file:

```bash
./webserv config/default.conf
```

---

### Example Usage

Open a browser and visit:

```
http://localhost:8080
```

You can also test using curl:

```bash
curl http://localhost:8080
```

Test POST request:

```bash
curl -X POST -d "name=test" http://localhost:8080
```

---

## Project Architecture

The project is typically structured as follows:

```
webserv/
├── config/
├── includes/
├── srcs/
├── www
├── www2
└── README.md
```

Core components include:

* Configuration parser
* Server Manager (handles multiple server instances)
* Client State (manages request lifecycle)
* Request parser
* Response builder
* CGI handler

---

## Technical Choices

* Language: C++ (C++98 standard)
* Multiplexing: select() / poll() / epoll() depending on implementation
* Non-blocking sockets
* Custom HTTP parser (no external libraries)
* Process fork/execve for CGI execution

The server is event-driven and single-threaded, relying on I/O multiplexing to handle multiple clients efficiently.

---

## Resources

### Documentation

- [RFC 7230 – Hypertext Transfer Protocol (HTTP/1.1)](https://datatracker.ietf.org/doc/html/rfc7230)
- [Linux man pages – socket](https://man7.org/linux/man-pages/man2/socket.2.html)
- [Linux man pages – poll](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Nginx Official Documentation](https://nginx.org/en/docs/)
- [C++98 Reference Documentation](https://en.cppreference.com/w/cpp?oldid=179926)
- [Socket Programming in C/C++ – GeeksforGeeks](https://www.geeksforgeeks.org/c/socket-programming-cc/)

### CGI Documentation

- [RFC 3875 – Common Gateway Interface](https://www.rfc-editor.org/rfc/rfc3875)
- [PHP CGI Documentation](https://www.php.net/manual/en/install.unix.commandline.php)

---

## AI Usage

AI tools were used during this project for:

* Reviewing code structure and suggesting architectural improvements
* Clarifying HTTP protocol behavior
* Generating the UI inerfaces

All implementation logic, architecture decisions, and final code were written and validated manually.

---

## Learning Outcomes

Through this project, the following concepts were mastered:

* Low-level network programming
* How HTTP works internally
* Multiplexing and non-blocking I/O
* Process management in Unix
* Web server architecture design
* Robust error handling
* Memory and resource management

This project provides a deep understanding of how real-world web servers function under the hood.

---

## License

This project is part of the 42 curriculum and is intended for educational purposes.
