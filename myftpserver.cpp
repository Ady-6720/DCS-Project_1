//Server.cpp
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <sstream>


#define BUFFER_SIZE 2048

using namespace std;

void sendResponse(int clientSocket, const char* response) {
    ssize_t bytesSent = send(clientSocket, response, strlen(response), 0);
    if (bytesSent == -1) {
        cerr << "Error: Failed to send response to client" << endl;
    }
}

void handleGet(int clientSocket, const string& filename) {
    ifstream file(filename.c_str(), ios::binary);
    if (!file.is_open()) {
        string errorMsg = "Error: File not found";
        sendResponse(clientSocket, errorMsg.c_str());
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = file.readsome(buffer, BUFFER_SIZE)) > 0) {
        ssize_t bytesSent = send(clientSocket, buffer, bytesRead, 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send file data" << endl;
            break;
        }
    }

    if (bytesRead == -1) {
        cerr << "Error: Failed to read file" << endl;
    }

    file.close();
}

void handlePut(int clientSocket, const char* filename) {
    // Debugging
    cout << "PUT command received for file: " << filename << endl;

    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Failed to open file for writing" << endl;
        // Send error response to client
        send(clientSocket, "Error: Failed to open file for writing", strlen("Error: Failed to open file for writing"), 0);
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesReceived;
    bool success = true; // Flag to track if the file transfer is successful

    cout << "Receive file content and write it to the file" << endl;
    while ((bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        // Write received data to file
        file.write(buffer, bytesReceived);
        
        // Check for end-of-file (client closed connection)
        if (bytesReceived < BUFFER_SIZE) {
            break;
        }
    }

    if (bytesReceived == -1) {
        cerr << "Error: Failed to receive file data" << endl;
        success = false;
    } else if (bytesReceived == 0) {
        cout << "Client closed the connection" << endl;
        success = false;
    }

    // Close the file
    file.close();

    // Send response to client
    if (success) {
        // Send success response to client
        send(clientSocket, "File received successfully", strlen("File received successfully"), 0);
    } else {
        // Send error response to client
        send(clientSocket, "Error: Failed to receive file data", strlen("Error: Failed to receive file data"), 0);
    }
}




void handleDelete(const char* filename, int clientSocket) {
    // Debugging
    //cout << "DELETE command received for file: " << filename << endl;
    
    if (remove(filename) != 0) {
        // Error deleting file
        string errorMsg = "Error: Unable to delete file";
        cerr << errorMsg << endl; // Log error to server console
        ssize_t bytesSent = send(clientSocket, errorMsg.c_str(), errorMsg.size(), 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send error message" << endl;
        }
    } else {
        // File deleted successfully
        string successMsg = "File deleted successfully";
        cout << successMsg << endl; // Log success to server console
        ssize_t bytesSent = send(clientSocket, successMsg.c_str(), successMsg.size(), 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send success message" << endl;
        }
    }
}


void handlePwd(int clientSocket) {
    // Debugging
    cout << "PWD command received" << endl;
    
    char path[1024];
    if (getcwd(path, sizeof(path)) != NULL) {
        sendResponse(clientSocket, path);
    } else {
        sendResponse(clientSocket, "Error: Unable to get current directory\n");
    }
}

void handleLs(int clientSocket) {
    // Debugging
    //cout << "LS command received" << endl;
    
    DIR* dir;
    struct dirent* ent;
    stringstream fileList;
    
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            fileList << ent->d_name << "\n";
        }
        closedir(dir);
        
        // Send the concatenated file list to the client
        string fileListStr = fileList.str();
        ssize_t bytesSent = send(clientSocket, fileListStr.c_str(), fileListStr.size(), 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send file list to client" << endl;
        }
    } else {
        sendResponse(clientSocket, "Error: Unable to open directory\n");
    }
}


void handleCd(int clientSocket, const char* dirname) {
    // Debugging
    cout << "CD command received to directory: " << dirname << endl;
    
    if (chdir(dirname) != 0) {
        string errorMsg = "Error: Unable to change directory";
        cerr << errorMsg << endl; // Log error to server console
        ssize_t bytesSent = send(clientSocket, errorMsg.c_str(), errorMsg.size(), 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send error message" << endl;
        }
    } else {
        // Directory changed successfully
        string successMsg = "Directory changed successfully";
        //cout << successMsg << endl; // Log success to server console
        ssize_t bytesSent = send(clientSocket, successMsg.c_str(), successMsg.size(), 0);
        if (bytesSent == -1) {
            cerr << "Error: Failed to send success message" << endl;
        }
    }
}


void handleMkdir(int clientSocket, const char* dirname) {
    // Debugging
    cout << "MKDIR command received for directory: " << dirname << endl;
    
    if (mkdir(dirname, 0777) != 0) {
        sendResponse(clientSocket, "Error: Unable to create directory\n");
    } else {
        sendResponse(clientSocket, "Directory created successfully\n");
    }
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[BUFFER_SIZE] = {0};
    socklen_t addrLen = sizeof(serverAddr);

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Error: Socket creation failed" << endl;
        return -1;
    }

    // Allow user to specify port number
    int port;
    cout << "Enter port number: ";
    cin >> port;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        cerr << "Error: Binding failed" << endl;
        return -1;
    }

    // Listen for connections
    if (listen(serverSocket, 5) == -1) {
        cerr << "Error: Listening failed" << endl;
        return -1;
    }

    cout << "Server started listening on port " << port << endl;

    // Accept incoming connections
    clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
    if (clientSocket == -1) {
        cerr << "Error: Connection acceptance failed" << endl;
        return -1;
    }
    cout << "Client connected" << endl;

    // Handle client commands
    while (true) {
        cout << "Waiting for command from client..." << endl;
        ssize_t bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived == -1) {
            cerr << "Error: Failed to receive command from client" << endl;
            break;
        } else if (bytesReceived == 0) {
            cout << "Client disconnected" << endl;
            break;
        }

        cout << "Command received: " << buffer << endl;

        if (strcmp(buffer, "quit") == 0) {
            cout << "Quitting session" << endl;
            break;
        }

        // Parse command and execute appropriate action
        char* token = strtok(buffer, " ");
        if (token != NULL) {
            if (strcmp(token, "get") == 0) {
                handleGet(clientSocket, strtok(NULL, " "));
            } else if (strcmp(token, "put") == 0) {
                handlePut(clientSocket, strtok(NULL, " "));
            } else if (strcmp(token, "delete") == 0) {
       		 handleDelete(strtok(NULL, " "), clientSocket);
	    } else if (strcmp(token, "pwd") == 0) {
		handlePwd(clientSocket);
		} else if (strcmp(token, "ls") == 0) {
		    handleLs(clientSocket);
		} else if (strcmp(token, "cd") == 0) {
		    handleCd(clientSocket, strtok(NULL, " "));
		} else if (strcmp(token, "mkdir") == 0) {
		    handleMkdir(clientSocket, strtok(NULL, " "));
		    } else {
                sendResponse(clientSocket, "Error: Unknown command\n");
            }
        }
        
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(serverSocket);
    close(clientSocket);

    return 0;
}
/*Yash Joshi
Aditya Malode*/
