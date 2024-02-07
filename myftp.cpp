//Client.cpp
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cerrno>

#define BUFFER_SIZE 2048

using namespace std;

void receiveResponse(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead == -1) {
        perror("recv");
        cerr << "Error: Failed to receive response from server" << endl;
    } else if (bytesRead == 0) {
        cerr << "Connection closed by server" << endl;
    } else {
        cout << "Server response: " << buffer << endl;
    }
}

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error: Socket creation failed" << endl;
        return -1;
    }

    // Allow user to specify IP address and port number
    char ip[20];
    cout << "Enter server IP address: ";
    cin.getline(ip, sizeof(ip));

    int port;
    cout << "Enter server port number: ";
    cin >> port;
    cin.ignore(); // Ignore newline character left in the input buffer

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    // Connect to server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error: Connection failed" << endl;
        return -1;
    }

    cout << "Connected to server" << endl;

    // Send commands to server
    while (true) {
        string command;
        cout << "Enter command (or 'quit' to exit): ";
        getline(cin, command);

        // Check for put command
        if (command.substr(0, 4) == "put ") {
            // Extract filename from the command
            string filename = command.substr(4);

            // Read file content
            ifstream file(filename.c_str(), ios::binary);
            if (!file.is_open()) {
                cerr << "Error: Failed to open file for reading" << endl;
                continue;
            }

            // Send the PUT command to the server
            ssize_t bytesSent = send(clientSocket, command.c_str(), command.size(), 0);
            if (bytesSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            // Read and send file content
            stringstream fileContent;
            fileContent << file.rdbuf();
            string fileContentStr = fileContent.str();
            ssize_t contentSent = send(clientSocket, fileContentStr.c_str(), fileContentStr.size(), 0);
            if (contentSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            cout << "File sent successfully" << endl;

            file.close();

            // Wait for server response
            receiveResponse(clientSocket);

            continue;
        }
        // Check for get command
        else if (command.substr(0, 4) == "get ") {
            // Send the GET command to the server
            ssize_t bytesSent = send(clientSocket, command.c_str(), command.size(), 0);
            if (bytesSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            // Receive response from server
            stringstream receivedContent;
            ssize_t bytesRead;
            bool errorOccurred = false;

            while (true) {
                bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead == -1) {
                    // Error handling for recv failure
                    perror("recv");
                    errorOccurred = true;
                    break;
                } else if (bytesRead == 0) {
                    // Connection closed by server
                    cout << "Connection closed by server" << endl;
                    break;
                }

                // Write received data to stringstream
                receivedContent.write(buffer, bytesRead);

                // Check if all data has been received
                if (bytesRead < BUFFER_SIZE) {
                    break;
                }
            }

            if (!errorOccurred) {
                // Process received content
                string content = receivedContent.str();
                //cout << "Received content from server:" << endl;
                //cout << content << endl;
            } else {
                // Handle error condition
                cerr << "Error: Failed to receive file content" << endl;
            }

            // Check if the while loop exited due to an error or socket closure
            if (bytesRead == -1) {
                cerr << "Error: Failed to receive file data" << endl;
                continue;
            } else if (bytesRead == 0) {
                cout << "Server closed the connection" << endl;
                break;
            }

            // Get filename from the command
            string filename = command.substr(4);
            // Write received content to a new file in the client's directory
            ofstream outputFile(filename.c_str(), ios::binary);
            if (!outputFile.is_open()) {
                cerr << "Error: Failed to open file for writing" << endl;
                continue;
            }
            outputFile << receivedContent.str();
            outputFile.close();
            cout << "File received successfully: " << filename << endl;

            continue; // Move to the next iteration of the loop
        }

        // Check for mkdir command
        if (command.substr(0, 6) == "mkdir ") {
            // Send the mkdir command to the server
            ssize_t bytesSent = send(clientSocket, command.c_str(), command.size(), 0);
            if (bytesSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            // Receive response from server
            receiveResponse(clientSocket);

            continue;
        }

        // Check for cd command
        if (command.substr(0, 3) == "cd ") {
            // Send the cd command to the server
            ssize_t bytesSent = send(clientSocket, command.c_str(), command.size(), 0);
            if (bytesSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            // Receive response from server
            receiveResponse(clientSocket);

            continue;
        }

        // Check for delete command
        if (command.substr(0, 7) == "delete ") {
            // Send the delete command to the server
            ssize_t bytesSent = send(clientSocket, command.c_str(), command.size(), 0);
            if (bytesSent == -1) {
                perror("send");
                cout << "Error code: " << errno << endl;
                // Handle error condition
                continue;
            }

            // Receive response from server
            receiveResponse(clientSocket);

            continue;
        }

        // Convert string to C-style string for sending
        strncpy(buffer, command.c_str(), BUFFER_SIZE);

        // Send command to server
        ssize_t bytesSent = send(clientSocket, buffer, strlen(buffer), 0);
        if (bytesSent == -1) {
            perror("send");
            cout << "Error code: " << errno << endl;
            // Handle error condition
        }

        // Receive response from server for specific commands
        if (command == "pwd" || command == "ls") {
            receiveResponse(clientSocket);
        }

        // Check if user wants to quit
        if (strcmp(buffer, "quit") == 0) {
            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // Clear buffer for next command
    }

    close(clientSocket);

    return 0;
}
//Yash Joshi
