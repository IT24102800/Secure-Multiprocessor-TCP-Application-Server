# Secure Multiprocessor TCP Application Server

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Python](https://img.shields.io/badge/Language-Python%203-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)

## 📌 Overview
This project was developed for the **Network Programming**. 

It is a fully functional, secure client-server application built from scratch. The system utilizes a custom TCP framing protocol, multiprocessing for concurrency, and robust security features including salted hashing, session tokens, and rate limiting.

- **Server:** Written in C (`server_2800.c`)
- **Client:** Written in Python 3 (`client_2800.py`)
- **Registration No:** IT24102800
- **Server ID (SID):** 4102
- **Port:** 50800

---

## ✨ Key Features

### 1. Custom TCP Protocol (Framing)
- Implements explicit framing using `LEN:<n>\n<payload>` headers.
- Handles partial `recv()` calls and multiple messages in a single buffer.
- Rejects invalid length headers and oversized payloads (> 4096 bytes).

### 2. Multiprocessing & Concurrency
- Uses `fork()` to create a separate child process for every connected client.
- Parent process continues to accept new connections (listen backlog of 10).
- Proper `SIGCHLD` handling using `waitpid()` to prevent zombie processes.

### 3. Authentication & Security
- **Salted Hashing:** Passwords are never stored in plaintext; a custom djb2 variant with salt is used.
- **Session Tokens:** Issues unique tokens upon login which expire after 5 minutes of inactivity.
- **Protected Commands:** Commands like `WHOAMI` and `LOGOUT` require a valid token.
- **Abuse Protection:** 
  - Rate limiting (5 requests/second per IP).
  - Brute-force lockout (3 failed attempts = 60-second lockout).
  - Strict username validation (3-20 chars, alphanumeric + underscores).

### 4. Persistent Audit Logging
- All client activities are logged to `server_IT24102800.log`.
- Logs include: Timestamp, Client IP:Port, Child PID, Username, Command, and Result.

---

## 📂 Project Structure

```text
├── server_2800.c           # Main C server implementation
├── client_2800.py          # Python CLI client
├── Makefile_2800           # Build automation
├── server_IT24102800.log   # Audit log (generated at runtime)
└── README.md               # Project documentation
```

---

## 🚀 Getting Started

### Prerequisites
- Linux environment (Kali Linux recommended)
- `gcc` compiler
- `python3`
- `sudo` privileges (required for binding to specific ports and creating directories in `/srv/`)

### 1. Build the Server
Use the provided Makefile to compile the server:
```bash
make -f Makefile_2800
```

### 2. Run the Server
Execute the server with `sudo` to ensure it can create the required data directories under `/srv/IE2102/IT24102800/`:
```bash
sudo ./server_2800
```
*Expected Output:* `Server running on port 50800, SID:4102`

### 3. Run the Client
Open a new terminal window and run the Python client:
```bash
python3 client_2800.py
```

---

## 💻 Client Commands

Once connected, the client supports the following commands:

| Command | Description |
| :--- | :--- |
| `REGISTER <user> <pass>` | Creates a new user account. |
| `LOGIN <user> <pass>` | Authenticates the user and returns a Session Token. |
| `LOGOUT` | Invalidates the current session token. |
| `WHOAMI` | Returns the current logged-in username (requires token). |
| `QUIT` | Closes the client connection gracefully. |

*Note: The client automatically stores the session token after a successful login and includes it in subsequent protected commands.*

---

## 🛡️ Security Test Cases (Proof of Concept)

1. **Brute Force Protection:** Attempting to login 3 times with the wrong password will lock the account for 60 seconds.
2. **Payload Overflow:** Sending a payload larger than 4096 bytes will be rejected by the server.
3. **Zombie Process Check:** Running `ps aux | grep defunct` after multiple connections will show zero zombie processes due to `waitpid()` implementation.

---

## 📸 Screenshots & Evidence
*(You can add your screenshots here, or link them if they are in an `/images` folder in your repo)*

1. **Hostname Configuration** - `hostnamectl`
2. **Network Configuration** - `ip addr`
3. **Server Running** - `make -f Makefile_2800` & `sudo ./server_2800`
4. **Successful REGISTER & LOGIN** - Client terminal output
5. **Failed Login & Lockout** - Client terminal output showing `ERR 403 Account locked`
6. **Multiple Forked Processes** - `ps aux | grep server_2800`
7. **Log File Entries** - `cat server_IT24102800.log`
8. **Proper Child Cleanup** - `ps aux | grep defunct` (No zombies found)

---

## ⚠️ Disclaimer
This project was developed for educational purposes as part of the IE2102 module at SLIIT. The code is not intended for production use without further security auditing.

---
**Author:** RAJAPAKSHA K.R.D.A.P.M. (IT24102800)  
**Module:** Network Programming  
```

### Tips for GitHub:
1. **Screenshots:** If you want the screenshots to show up in the README, create a folder called `images` in your repository, upload your screenshots there, and then replace the text in the "Screenshots & Evidence" section with actual image links (e.g., `![Server Running](images/server_running.png)`).
2. **Badges:** The badges at the top are standard shield.io badges. They make the repo look professional.
3. **.gitignore:** Don't forget to add a `.gitignore` file to your repo to ignore compiled binaries (like `server_2800`) and log files (like `server_IT24102800.log`) so they don't clutter your repository.
