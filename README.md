# Secure-Multiprocessor-TCP-Application-Server
A secure, concurrent client-server application built from scratch as part of the Network Programming module.  This project implements a custom TCP protocol with explicit framing, multiprocessing for concurrency, and robust security features including salted hashing, session tokens, and abuse protection.

📑 Table of Contents
Features

Project Structure

Technical Architecture

How to Build & Run

Usage & Commands

Security Implementations

Screenshots & Evidence

✨ Features
Server (C)
Multiprocessing: Uses fork() to handle multiple concurrent clients simultaneously.

Custom Protocol: Implements explicit framing using LEN:<n>\n<payload> to handle partial recv() and multiple messages in one buffer.

Robust Signal Handling: Handles SIGCHLD with waitpid() to prevent zombie processes.

Persistent Audit Logging: Records all client activities (timestamp, IP, PID, command, result) to a log file.

Data Validation: Enforces username constraints (3-20 alphanumeric/underscore) and rejects payloads >4096 bytes.

Client (Python)
Command-Line Interface: User-friendly CLI for interacting with the server.

Protocol Compliance: Automatically formats commands with LEN:<n> headers.

Session Management: Automatically stores and appends session tokens for protected commands.

Graceful Shutdown: Sends a LOGOUT command before QUIT if a user is logged in.

Security & Abuse Protection
Salted Hashing: Passwords are never stored in plaintext; uses a djb2 variant with salting.

Session Tokens: Secure tokens issued on login with a 5-minute inactivity expiry.

Rate Limiting: Restricts requests to 5 requests/second per IP.

Brute-Force Lockout: Locks accounts for 60 seconds after 3 consecutive failed login attempts.

📂 Project Structure
text
├── server_2800.c          # Main C server source code
├── client_2800.py         # Python client source code
├── Makefile_2800          # Build automation for the server
├── server_IT24102800.log  # Persistent audit log (generated at runtime)
└── README.md              # Project documentation
(Note: Data is stored under /srv/IE2102/IT24102800/RAJAPAKSHA_K.R.D.A.P.M as per assignment requirements.)

🏗️ Technical Architecture
1. Custom TCP Framing
To overcome TCP stream fragmentation, the protocol uses a length-prefixed header:

Client Request: LEN:<n>\n<payload>

Server Response: OK <code> SID:4102 <message> or ERR <code> SID:4102 <message>

The server correctly handles partial reads, multiple concatenated messages, invalid lengths, and oversized payloads (>4096 bytes).

2. Multiprocessing Model
The parent process listens for connections.

Upon accept(), it calls fork() to create a child process.

The child handles the client session independently.

The parent continues accepting new connections and cleans up terminated children using waitpid().

🚀 How to Build & Run
Prerequisites
Linux OS (Tested on Kali Linux)

gcc compiler

python3

sudo privileges (required for binding to ports and writing to /srv/)

Build and Start the Server
bash
# Compile and run using the Makefile
make -f Makefile_2800
Alternatively, compile manually:

bash
sudo gcc -Wall -o server_2800 server_2800.c
sudo ./server_2800
Expected Output: Server running on port 50800, SID:4102

Run the Client
Open a new terminal window:

bash
python3 client_2800.py
💻 Usage & Commands
Once connected, the client CLI supports the following commands:

Command	Description	Requires Token?
REGISTER <user> <pass>	Creates a new account	No
LOGIN <user> <pass>	Authenticates and returns a session token	No
WHOAMI	Returns the currently logged-in username	Yes
LOGOUT	Invalidates the current session token	Yes
QUIT	Closes the client connection	No
🔒 Security Implementations
No Plaintext Storage: Passwords are hashed using a salted djb2 algorithm before being stored.

Token Expiry: Tokens are validated on every protected request and expire after 5 minutes of inactivity.

Abuse Prevention:

Rate Limiting: Protects against DDoS/flooding (max 5 req/sec per IP).

Lockout: Protects against dictionary attacks (3 failed attempts = 60s lockout).

📸 Screenshots & Evidence
(Include 1-2 of your best screenshots here from your report to make the README visually appealing)

Feature	Evidence
Multiprocessing	(Insert ps aux | grep server_2800 screenshot here)
Failed Login & Lockout	(Insert lockout screenshot here)
Zero Zombie Processes	(Insert ps aux | grep defunct output here)
