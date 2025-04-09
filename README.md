# Chat Application (Terminal-Based)

A real-time terminal-based chat application built in C using POSIX sockets and `ncurses`.  
Supports multiple users, interactive UI, input history, and basic command handling.

## Features

- Multi-client chat support via a central server
- Interactive terminal UI using `ncurses`
- Real-time message broadcasting to all clients
- Username registration
- Graceful exit using `/quit` or `Ctrl+C`

## Structure

- `server.c` – Manages client connections and message broadcasting
- `client.c` – Terminal UI client with `ncurses` interface
- `common.h` – Shared constants, includes, and function declarations

## How to Run

### 1. Clone the Repository

```bash
git clone https://github.com/deshnalovestats/chat_application.git
cd chat_application
```

### 2. Compile the Server and Client
```bash
gcc server.c -o server -lpthread
gcc client.c -o client -lncurses -lpthread
```
### 3. Run the Server 
```bash
./server
```

### 4. Run the client
Open multiple different terminals and run,
  ```bash
  ./client
