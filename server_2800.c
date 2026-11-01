#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>

#define PORT 50800
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10
#define TOKEN_EXPIRE_SEC 300
#define MAX_LOGIN_ATTEMPTS 3
#define LOCKOUT_SEC 60

typedef struct {
    char username[50];
    char salt[20];
    char hash[100];
    int failed_attempts;
    int locked_until;
} User;

typedef struct {
    char token[100];
    char username[50];
    time_t last_active;
} Session;

User users[100];
Session sessions[100];
int user_count = 0;
int session_count = 0;
FILE *log_file;
char base_path[256];

void hash_password(const char *pass, const char *salt, char *out) {
    unsigned long hash = 5381;
    char combined[150];
    sprintf(combined, "%s%s", pass, salt);
    for (char *ptr = combined; *ptr; ptr++)
        hash = ((hash << 5) + hash) + *ptr;
    sprintf(out, "%lx", hash);
}

void generate_salt(char *salt) {
    const char *chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 16; i++)
        salt[i] = chars[rand() % 36];
    salt[16] = '\0';
}

void generate_token(char *token, const char *username) {
    sprintf(token, "%lx_%s_%lx", (unsigned long)rand(), username, (unsigned long)time(NULL));
}

int validate_username(const char *username) {
    int len = strlen(username);
    if (len < 3 || len > 20) return 0;
    for (int i = 0; i < len; i++)
        if (!isalnum(username[i]) && username[i] != '_') return 0;
    return 1;
}

void save_users() {
    char filepath[512];
    sprintf(filepath, "%s/users.dat", base_path);
    FILE *f = fopen(filepath, "wb");
    if (f) {
        fwrite(&user_count, sizeof(int), 1, f);
        fwrite(users, sizeof(User), user_count, f);
        fclose(f);
    }
}

void load_users() {
    char filepath[512];
    sprintf(filepath, "%s/users.dat", base_path);
    FILE *f = fopen(filepath, "rb");
    if (f) {
        fread(&user_count, sizeof(int), 1, f);
        fread(users, sizeof(User), user_count, f);
        fclose(f);
    }
}

User* find_user(const char *username) {
    for (int i = 0; i < user_count; i++)
        if (strcmp(users[i].username, username) == 0)
            return &users[i];
    return NULL;
}

Session* find_session_by_token(const char *token) {
    for (int i = 0; i < session_count; i++)
        if (strcmp(sessions[i].token, token) == 0)
            return &sessions[i];
    return NULL;
}

int validate_session(const char *token) {
    time_t now = time(NULL);
    Session *s = find_session_by_token(token);
    if (s) {
        if (now - s->last_active < TOKEN_EXPIRE_SEC) {
            s->last_active = now;
            return 1;
        }
    }
    return 0;
}

void log_event(const char *ip, int port, int pid, const char *user, const char *cmd, const char *result) {
    time_t now = time(NULL);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log_file, "[%s] IP:%s:%d PID:%d USER:%s CMD:%s RESULT:%s\n", ts, ip, port, pid, user, cmd, result);
    fflush(log_file);
}

int check_rate_limit(const char *ip) {
    static struct { char ip[16]; time_t last; int count; } rate[100];
    static int rate_count = 0;
    time_t now = time(NULL);
    
    for (int i = 0; i < rate_count; i++) {
        if (strcmp(rate[i].ip, ip) == 0) {
            if (now - rate[i].last < 1) {
                if (++rate[i].count > 5) return 0;
            } else {
                rate[i].count = 1;
            }
            rate[i].last = now;
            return 1;
        }
    }
    if (rate_count < 100) {
        strcpy(rate[rate_count].ip, ip);
        rate[rate_count].last = now;
        rate[rate_count].count = 1;
        rate_count++;
    }
    return 1;
}

void send_response(int fd, const char *code, const char *msg, int is_ok) {
    char resp[1024];
    if (is_ok) {
        snprintf(resp, sizeof(resp), "OK %s SID:4102 %s\r\n", code, msg);
    } else {
        snprintf(resp, sizeof(resp), "ERR %s SID:4102 %s\r\n", code, msg);
    }
    send(fd, resp, strlen(resp), 0);
}

void process_message(char *payload, int fd, char *ip, int port, int pid, char *current_user, char *current_token) {
    char cmd[20], arg1[50], arg2[50];
    int parsed = sscanf(payload, "%s %s %s", cmd, arg1, arg2);
    
    if (parsed < 1) {
        send_response(fd, "400", "Invalid command", 0);
        log_event(ip, port, pid, current_user, "INVALID", "Failed");
        return;
    }
    
    for (int i = 0; cmd[i]; i++) cmd[i] = toupper(cmd[i]);
    
    if (strcmp(cmd, "REGISTER") == 0) {
        if (parsed < 3) {
            send_response(fd, "400", "Need username and password", 0);
        } else if (!validate_username(arg1)) {
            send_response(fd, "400", "Invalid username", 0);
        } else if (find_user(arg1)) {
            send_response(fd, "409", "User exists", 0);
        } else {
            User *u = &users[user_count++];
            strcpy(u->username, arg1);
            generate_salt(u->salt);
            hash_password(arg2, u->salt, u->hash);
            u->failed_attempts = 0;
            u->locked_until = 0;
            
            char user_path[512];
            sprintf(user_path, "%s/%s", base_path, arg1);
            mkdir(user_path, 0755);
            
            save_users();
            send_response(fd, "200", "Registration successful", 1);
            log_event(ip, port, pid, arg1, cmd, "Success");
        }
    }
    else if (strcmp(cmd, "LOGIN") == 0) {
        if (parsed < 3) {
            send_response(fd, "400", "Need username and password", 0);
        } else {
            User *u = find_user(arg1);
            if (!u) {
                send_response(fd, "401", "Invalid credentials", 0);
                log_event(ip, port, pid, arg1, cmd, "Failed");
            } else if (u->locked_until > time(NULL)) {
                send_response(fd, "403", "Account locked", 0);
                log_event(ip, port, pid, arg1, cmd, "Failed");
            } else {
                char test_hash[100];
                hash_password(arg2, u->salt, test_hash);
                if (strcmp(test_hash, u->hash) == 0) {
                    u->failed_attempts = 0;
                    generate_token(current_token, arg1);
                    strcpy(current_user, arg1);
                    
                    Session *s = &sessions[session_count++];
                    strcpy(s->token, current_token);
                    strcpy(s->username, arg1);
                    s->last_active = time(NULL);
                    
                    char resp[1024];
                    snprintf(resp, sizeof(resp), "OK 200 SID:4102 Login successful\r\nTOKEN:%s\r\n", current_token);
                    send(fd, resp, strlen(resp), 0);
                    log_event(ip, port, pid, arg1, cmd, "Success");
                } else {
                    u->failed_attempts++;
                    if (u->failed_attempts >= MAX_LOGIN_ATTEMPTS)
                        u->locked_until = time(NULL) + LOCKOUT_SEC;
                    save_users();
                    send_response(fd, "401", "Invalid credentials", 0);
                    log_event(ip, port, pid, arg1, cmd, "Failed");
                }
            }
        }
    }
    else if (strcmp(cmd, "LOGOUT") == 0) {
        if (strlen(current_token) > 0 && validate_session(current_token)) {
            for (int i = 0; i < session_count; i++) {
                if (strcmp(sessions[i].token, current_token) == 0) {
                    sessions[i] = sessions[--session_count];
                    break;
                }
            }
            current_token[0] = '\0';
            strcpy(current_user, "unauthenticated");
            send_response(fd, "200", "Logout successful", 1);
            log_event(ip, port, pid, current_user, cmd, "Success");
        } else {
            send_response(fd, "401", "Not logged in", 0);
        }
    }
    else if (strcmp(cmd, "WHOAMI") == 0) {
        if (strlen(current_token) > 0 && validate_session(current_token)) {
            char msg[100];
            snprintf(msg, sizeof(msg), "You are: %s", current_user);
            send_response(fd, "200", msg, 1);
            log_event(ip, port, pid, current_user, cmd, "Success");
        } else {
            send_response(fd, "401", "Not logged in", 0);
        }
    }
    else if (strcmp(cmd, "QUIT") == 0) {
        send_response(fd, "200", "Goodbye", 1);
        log_event(ip, port, pid, current_user, cmd, "Success");
        close(fd);
        exit(0);
    }
    else {
        send_response(fd, "400", "Unknown command", 0);
        log_event(ip, port, pid, current_user, cmd, "Unknown");
    }
}

void handle_client(int fd, struct sockaddr_in addr, int pid) {
    char buffer[BUFFER_SIZE], ip[16], user[50] = "unauthenticated", token[100] = "";
    char recv_buf[BUFFER_SIZE * 2];
    int port = ntohs(addr.sin_port), recv_len = 0;
    strcpy(ip, inet_ntoa(addr.sin_addr));
    
    while (1) {
        int n = recv(fd, buffer, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;
        
        memcpy(recv_buf + recv_len, buffer, n);
        recv_len += n;
        
        char *ptr = recv_buf;
        while (recv_len >= 4 && strncmp(ptr, "LEN:", 4) == 0) {
            int declared_len = atoi(ptr + 4);
            if (declared_len <= 0 || declared_len > 4096) {
                send_response(fd, "400", "Invalid length", 0);
                int skip = 0;
                while (recv_len > skip && ptr[skip] != '\n' && skip < 100) skip++;
                if (skip < recv_len) skip++;
                ptr += skip;
                recv_len -= skip;
                continue;
            }
            
            char *newline = strstr(ptr, "\n");
            if (!newline) break;
            newline++;
            
            int header_len = newline - ptr;
            int total_expected = header_len + declared_len;
            
            if (recv_len < total_expected) break;
            
            char *payload = newline;
            char payload_copy[4097];
            strncpy(payload_copy, payload, declared_len);
            payload_copy[declared_len] = '\0';
            
            if (!check_rate_limit(ip)) {
                send_response(fd, "429", "Rate limit exceeded", 0);
            } else {
                process_message(payload_copy, fd, ip, port, pid, user, token);
            }
            
            ptr += total_expected;
            recv_len -= total_expected;
        }
        
        if (recv_len > 0 && ptr != recv_buf) {
            memmove(recv_buf, ptr, recv_len);
        }
    }
    close(fd);
    exit(0);
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    sprintf(base_path, "/srv/ie2102/IT24102800");
    mkdir(base_path, 0755);
    
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
    
    srand(time(NULL));
    log_file = fopen("server_IT24102800.log", "a");
    if (!log_file) log_file = stderr;
    
    load_users();
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, MAX_CLIENTS);
    
    printf("Server running on port %d, SID:4102\n", PORT);
    log_event("SYSTEM", 0, getpid(), "SYSTEM", "START", "Server started");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        
        pid_t child_pid = fork();
        if (child_pid == 0) {
            close(server_fd);
            handle_client(client_fd, client_addr, getpid());
        } else {
            close(client_fd);
        }
    }
    close(server_fd);
    fclose(log_file);
    return 0;
}
