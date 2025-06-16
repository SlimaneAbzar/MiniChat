// Allows compiler to use older/deprecated Winsock functions
#define _WINSOCK_DEPRECATED_NO_WARNINGS

// winsock2.h library for sockets, tcp, ip
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Link the program with the Winsock library
#pragma comment(lib, "ws2_32.lib")

#define PORT 5000         
#define MAX_CLIENTS 10     
#define BUF_SIZE 512      
#define MAX_LEN 1028 

int main() {
    // Structure to hold Winsock data
    WSADATA wsa;

    // server_socket: accept new clients
    // client_socket: for each new connection
    // client_sockets: store socket of connected clients
    SOCKET server_socket, client_socket, client_sockets[MAX_CLIENTS];

    // Structure to hold server and client address information
    struct sockaddr_in server, client;

    // Variable for size of client structure
    int addrlen = sizeof(client);

    // Temp buffer for received client data
    char buffer[BUF_SIZE];

    // manage socket connections with select()
    fd_set readfds;

    // i: client index, j: loop for broadcasting, k: character index
    int s_activityCount, byt_received, currentSocket, max_cS, i, j, k;

    // Each client gets own message buffer 
    char client_buffers[MAX_CLIENTS][MAX_LEN];     // Message buffers for each client
    int client_buf_lens[MAX_CLIENTS] = {0};         // Length of data currently stored in each buffer

    // Initialise sockets and buffers
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;                         // No client connected
        client_buf_lens[i] = 0;                        // No data inbuffer
        memset(client_buffers[i], 0, MAX_LEN);        // Clear buffer
    }

    // Start the Winsock library v.2.2
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
        return 1; // Error handling 
    }

    // Create the server socket
    // AF_INET to use IPv4
    // SOCK_STREAM to use TCP connection
    // 0 to use default protocol for TCP
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed. Error: %d\n", WSAGetLastError());
        WSACleanup(); // Clean up Winsock
        return 1;
    }

    // Fill in server address structure
    server.sin_family = AF_INET;                // IPv4
    server.sin_addr.s_addr = INADDR_ANY;        // Accept connections from local IP address's
    server.sin_port = htons(PORT);              // Convert port number to network byte order

    // Attach server socket to IP and port set
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed. Error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) { // MAX_CLIENTS: queue size for waiting connections
        printf("Listen failed. Error: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    printf("MiniChat server started on port %d\n", PORT);

    // Main server loop
    while (1) {
        // clear socket set
        FD_ZERO(&readfds);

        // Add main server socket to set to detect new incoming connection requests
        FD_SET(server_socket, &readfds);
        max_cS = server_socket;

        // Add each client socket to the set if it's active - non zero
        for (i = 0; i < MAX_CLIENTS; i++) {
            currentSocket = client_sockets[i];
            if (currentSocket > 0) {
                FD_SET(currentSocket, &readfds); // Add current client's socket to the monitoring set
            }
            if (currentSocket > max_cS) {
                max_cS = currentSocket; // Keep track of the largest socket value
            }
        }

        // Wait for s_activityCount on any of the sockets until one is ready
        s_activityCount = select(max_cS + 1, &readfds, NULL, NULL, NULL);

        if (s_activityCount == SOCKET_ERROR) {
            printf("select error. Error: %d\n", WSAGetLastError());
            break; // Error handling
        }

        // Check if there is a new connection request to the server
        if (FD_ISSET(server_socket, &readfds)) {
            client_socket = accept(server_socket, (struct sockaddr*)&client, &addrlen);
            if (client_socket == INVALID_SOCKET) {
                printf("accept failed. Error: %d\n", WSAGetLastError());
                continue; // Go back and wait again
            }

            // Find the first empty slot in the client_sockets array to store the new client
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_socket;
                    client_buf_lens[i] = 0;
                    memset(client_buffers[i], 0, MAX_LEN);

                    // Print information about the new client to terminal
                    printf("New client connected, socket fd is %d, ip: %s, port: %d\n",
                           (int)client_socket, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                    break;
                }
            }

            // If the server is already full, reject the connection
            if (i == MAX_CLIENTS) {
                printf("Max clients reached. Rejecting client.\n");
                closesocket(client_socket); // Close the rejected connection
            }
        }

        // Loop through all  client sockets to check if they sent data
        for (i = 0; i < MAX_CLIENTS; i++) {
            currentSocket = client_sockets[i];

            if (currentSocket > 0 && FD_ISSET(currentSocket, &readfds)) {
                // Receive data from the client into the buffer
                byt_received = recv(currentSocket, buffer, BUF_SIZE, 0);

                if (byt_received == 0) {
                    // If recv() returns 0, the client disconnected
                    printf("Client disconnected, socket fd is %d\n", (int)currentSocket);
                    closesocket(currentSocket);
                    client_sockets[i] = 0;
                    client_buf_lens[i] = 0;
                    memset(client_buffers[i], 0, MAX_LEN);
                } else if (byt_received > 0) {
                    // Process each character received
                    for (k = 0; k < byt_received; k++) {
                        char c = buffer[k];

                        // Store the character in the client's message buffer
                        if (client_buf_lens[i] < MAX_LEN - 1) {
                            client_buffers[i][client_buf_lens[i]++] = c;
                        }

                        // If the character is a newline or carriage return, we treat it as end of message
                        if (c == '\n' || c == '\r') {
                            client_buffers[i][client_buf_lens[i]] = '\0'; // Null terminate the string

                            // Broadcast the completed message to all connected clients
                            for (j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] != 0) {
                                    send(client_sockets[j], client_buffers[i], client_buf_lens[i], 0);
                                }
                            }

                            // Reset the buffer for this client
                            client_buf_lens[i] = 0;
                            memset(client_buffers[i], 0, MAX_LEN);
                        }
                    }
                }
            }
        }
    }

    // After the main loop ends, clean up all the sockets
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0) {
            closesocket(client_sockets[i]);
        }
    }

    // Close the server socket and clean up Winsock
    closesocket(server_socket);
    WSACleanup();
    return 0; // End of program
}

