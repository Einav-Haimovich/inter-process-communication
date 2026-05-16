# Inter-Process Communication — Signal + File Messaging

Client-server IPC in C using Unix signals for notification and text files as the message channel. The server performs arithmetic on behalf of clients; each client sends one request and waits for the result.

---

## Components

**[server.c](server.c)**
Waits for `SIGUSR1` from any client. On receipt, reads `toServer.txt`, forks a child process to perform the calculation, writes the result to `{clientPID}_toClient.txt`, and signals the client back. Exits after 60 seconds of silence.
_Learned: `fork()`-per-request isolates the calculation so the parent can keep listening — the child writes the result file and signals the client, while the parent just `wait()`s._

---

**[client.c](client.c)**
Takes `serverPID num1 operation num2` as arguments. After a random delay (0-5s), writes the request to `toServer.txt` and sends `SIGUSR1` to the server. Blocks until `SIGUSR1` comes back, then reads its response file and exits. Times out after 30 seconds.
_Learned: `O_EXCL` on `open()` is the POSIX way to atomically claim a file — if two clients race to write `toServer.txt`, only one succeeds; the other gets an error and retries._

---

## IPC Mechanisms Used

**SIGUSR1** — the notification channel between client and server (request and response).
**SIGALRM** — timeout watchdog (server: 60s, client: 30s) so processes don't hang forever.
**toServer.txt** — one shared request file; `O_EXCL` enforces mutual exclusion on writes.
**{clientPID}_toClient.txt** — per-client response file, named by PID to avoid collisions.
**fork()** — server spawns one child per request so it can return to listening immediately.

---

## How to Run

```bash
# Compile
gcc -o server server.c
gcc -o client client.c

# Terminal 1: start the server, note its PID
./server &
echo $!

# Terminal 2: run a client (op: 1=+, 2=-, 3=*, 4=/)
./client <serverPID> <num1> <op> <num2>

# Example: ask the server to compute 10 + 3
./client 12345 10 1 3
```

---

## Folder Structure

```
inter-process-communication/
├── server.c    # Signal handler + fork-per-request server
└── client.c    # Random-delay client with retry and timeout logic
```
