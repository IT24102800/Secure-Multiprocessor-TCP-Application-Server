import socket
import re

PORT = 50800
SERVER = "127.0.0.1"

def send_command(sock, cmd, token=None):
    if token:
        cmd = cmd + " TOKEN:" + token
    payload = cmd + "\n"
    msg = f"LEN:{len(payload)}\n{payload}"
    sock.send(msg.encode())
    
    response = ""
    while True:
        chunk = sock.recv(4096).decode()
        if not chunk:
            break
        response += chunk
        if "\r\n" in response:
            lines = response.split("\r\n")
            if len(lines) >= 1 and (lines[0].startswith("OK") or lines[0].startswith("ERR")):
                break
    return response.strip()

def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((SERVER, PORT))
    
    token = None
    
    print("=" * 50)
    print("Connected to Server (SID:4102)")
    print("=" * 50)
    print("Commands:")
    print("  REGISTER <user> <pass>")
    print("  LOGIN <user> <pass>")
    print("  LOGOUT")
    print("  WHOAMI")
    print("  QUIT")
    print("-" * 50)
    
    while True:
        cmd = input("\n> ").strip()
        if not cmd:
            continue
        
        parts = cmd.split()
        cmd_upper = parts[0].upper()
        
        if cmd_upper == "QUIT":
            if token:
                send_command(sock, "LOGOUT", token)
            print("Goodbye!")
            break
        
        elif cmd_upper == "REGISTER":
            if len(parts) != 3:
                print("Usage: REGISTER <username> <password>")
            else:
                resp = send_command(sock, cmd)
                print(resp)
        
        elif cmd_upper == "LOGIN":
            if len(parts) != 3:
                print("Usage: LOGIN <username> <password>")
            else:
                resp = send_command(sock, cmd)
                print(resp)
                if "OK" in resp and "200" in resp:
                    match = re.search(r'TOKEN:(\S+)', resp)
                    if match:
                        token = match.group(1)
                        print("[Token received]")
        
        elif cmd_upper == "LOGOUT":
            if token:
                resp = send_command(sock, "LOGOUT", token)
                print(resp)
                if "OK" in resp:
                    token = None
            else:
                print("Not logged in")
        
        elif cmd_upper == "WHOAMI":
            if token:
                resp = send_command(sock, "WHOAMI", token)
                print(resp)
            else:
                print("Not logged in")
        
        else:
            print("Unknown command")
    
    sock.close()

if __name__ == "__main__":
    main()
