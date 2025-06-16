# MiniChat
MiniChat is a simple TCP chat server built in C using Winsock2 on Windows. It supports multiple clients and broadcasts messages to all the clients.

---

## Features

- Supports up to 10 clients
- Uses TCP sockets with Winsock2
- Clients connect via Telnet in real time
- Handles basic message buffering and broadcasting
- Runs on Windows and requires Winsock2 to work 

---

## How It Works

The server listens on port **5000** for incoming connections. Each client sends a message terminated by a newline and it is then broadcast between all connected users.

---

## Setup Instructions

### 1. Enable Telnet Client on Windows

MiniChat clients connect via Telnet, which is not enabled by default on Windows.

- Open **Control Panel**
- Navigate to **Programs**
- Click **Turn Windows features on or off**
- Scroll down and check the box for **Telnet Client**
- Click **OK** and wait for Windows to install it

---

### 2. How to run

- Compile the C code
- Run the MiniChat.exe file
- You should see "MiniChat server started on port 5000"
- Open a new terminal and enter "telnet 127.0.0.1 5000"
- This will connect a new client to the server
- Type messages and press enter to send
- Each client should then receive the message

