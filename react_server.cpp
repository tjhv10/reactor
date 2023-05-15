#include <iostream>
#include <functional>
#include <dlfcn.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "st_Reactor.so"

using handler_t = std::function<void(int)>;
using namespace std;
void handleClientData(int client_fd) {
    // Buffer to store received data from the client
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    // Read data from the client socket
    ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        std::cerr << "Failed to read data from client socket" << std::endl;
        return;
    }

    if (bytesRead == 0) {
        // Client closed the connection
        std::cout << "Client disconnected" << std::endl;
        close(client_fd);
        return;
    }

    // Process the received data
    std::cout << "Received data from client: " << buffer << std::endl;
}


int main() {
    // Load the st_reactor.so library
    void* reactorLibHandle = dlopen("./st_reactor.so", RTLD_NOW);
    if (!reactorLibHandle) {
        std::cerr << "Failed to load st_reactor.so: " << dlerror() << std::endl;
        return 1;
    }

    // Function pointers to the library functions
    using CreateReactorFunc = void* (*)();
    using AddFdFunc = void (*)(handler_t, int, void*);
    using StartReactorFunc = void (*)(void*);
    using WaitForFunc = void (*)(void*);
    using StopReactorFunc = void (*)(void*);

    // Retrieve function pointers from the library
    auto createReactorFunc = (CreateReactorFunc)dlsym(reactorLibHandle, "createReactor");
    auto addFdFunc = (AddFdFunc)dlsym(reactorLibHandle, "addFd");
    StartReactorFunc startReactorFunc = (StartReactorFunc)dlsym(reactorLibHandle, "startReactor");
    WaitForFunc waitForFunc = (WaitForFunc)dlsym(reactorLibHandle, "waitFor");
    StopReactorFunc stopReactorFunc = (StopReactorFunc)dlsym(reactorLibHandle, "stopReactor");

    // Create the Reactor
    void *reactor = createReactorFunc();
    if (!reactor) {
        std::cerr << "Failed to create Reactor" << std::endl;
        dlclose(reactorLibHandle);
        return 1;
    }

    // Add the server socket FD to the Reactor
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create server socket" << std::endl;
        return 1;
    }
    addFdFunc(handleClientData, serverSocket, reactor);
    // Create a socket for the server
    

    // Set up server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);  // Replace with your desired port number

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to bind server socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen for client connections
    if (listen(serverSocket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on server socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server started. Listening for connections..." << std::endl;

    //add the server socket FD to reactor
    addFdFunc(handleClientData, serverSocket, reactor);

    // Start the Reactor
    startReactorFunc(reactor);

    // Accept and handle client connections
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        // Convert client IP address to string format
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIp, INET_ADDRSTRLEN);
        std::cout << "Accepted connection from: " << clientIp << std::endl;

        // Add the client socket FD to the Reactor
        addFdFunc(handleClientData, clientSocket, reactor);
    }
    // Start the Reactor thread
    startReactorFunc(reactor);


    // Stop the Reactor
    stopReactorFunc(reactor);

    // Wait for the Reactor thread to finish
    waitForFunc(reactor);

    // Cleanup
    dlclose(reactorLibHandle);

    return 0;
}
