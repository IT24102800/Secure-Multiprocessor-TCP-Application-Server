# Secure-Multiprocessor-TCP-Application-Server
A secure, concurrent client-server application built from scratch as part of the Network Programming module.  This project implements a custom TCP protocol with explicit framing, multiprocessing for concurrency, and robust security features including salted hashing, session tokens, and abuse protection.

📖 Overview
This project is a fully functional, secure Client-Server application built from scratch. It implements a custom TCP protocol, multiprocessing for concurrency, and robust security features including salted password hashing, session tokens, and brute-force protection.

The system consists of a C-based server (handling low-level socket operations and process forking) and a Python-based client (providing a user-friendly CLI interface).

✨ Key Features
🖧 Network & Protocol
Custom TCP Protocol: Implements explicit framing using LEN:<n> headers to handle partial recv() calls and multiple messages in a single buffer.

Multiprocessing: Uses fork() to spawn a new child process for every client connection, allowing concurrent handling of multiple clients.

Zombie Prevention: Implements proper SIGCHLD handling with waitpid() to ensure clean child process termination and prevent zombie processes.

Payload Validation: Rejects oversized payloads (>4096 bytes) and invalid length headers.

🔒 Security & Authentication
Salted Hashing: Passwords are never stored in plaintext. Uses a custom salted hashing algorithm (djb2 variant) for secure storage.

Session Management: Issues unique session tokens upon successful login. Tokens expire after 5 minutes of inactivity.

Protected Commands: Commands like WHOAMI and LOGOUT require a valid session token.

Abuse Protection:

Rate Limiting: Restricts clients to 5 requests per second per IP.

Brute-Force Lockout: Locks accounts for 60 seconds after 3 consecutive failed login attempts.

Input Validation: Enforces strict username rules (3-20 alphanumeric characters and underscores).

📝 Audit Logging
Maintains a persistent audit log (server_IT24102800.log) recording all activities with timestamps, client IP:Port, Child PID, Username, Command, and Result.

🛠️ Technology Stack
Server: C (Compiled with gcc)

Client: Python 3

Environment: Linux (Kali/Ubuntu)

Build System: Makefile

📂 Project Structure
text
├── server_2800.c          # Main server source code (C)
├── client_2800.py         # Client CLI interface (Python)
├── Makefile_2800          # Build automation script
├── server_IT24102800.log  # Generated audit log file
└── README.md              # Project documentation
🚀 Getting Started
Prerequisites
Linux OS (Tested on Kali Linux)

gcc compiler

python3

1. Build and Run the Server
The Makefile automates the compilation and execution. You need sudo privileges to bind to the specific port and write to the /srv/ directory.

bash
# Compile and run the server
make -f Makefile_2800
The server will start listening on port 50800.

2. Run the Client
Open a new terminal window and run the Python client:

bash
python3 client_2800.py
💻 Usage & Commands
Once the client is connected, you can use the following commands:

Command	Description	Example
REGISTER <user> <pass>	Creates a new user account.	REGISTER john 123
LOGIN <user> <pass>	Authenticates and returns a session token.	LOGIN john 123
WHOAMI	Returns the current logged-in username.	WHOAMI
LOGOUT	Invalidates the current session token.	LOGOUT
QUIT	Closes the client connection.	QUIT
📸 Proof of Implementation
(Include the screenshots from your report here on GitHub to show evidence of your work)

Network Configuration: Proof of IP address and port binding.

Multiple Forked Processes: ps aux showing the parent and child processes.

Security Lockout: Demonstration of the 60-second lockout after 3 failed attempts.

Audit Logs: Screenshot of the server_IT24102800.log file.

No Zombies: Proof of clean process termination.
