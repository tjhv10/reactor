#include <iostream>
#include <functional>
#include <dlfcn.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "st_Reactor.so"

#define PORT 9034
#define BUFF_SIZE 1024
using namespace std;
using handler_t = function<int(int)>;
handler_t handleClientData(int client_fd)
{
    return [client_fd](int /*unused*/) -> int
    {
        cout << "Handling client fd: " << client_fd << endl;

        // Buffer to store received data from the client
        char buffer[BUFF_SIZE];
        memset(buffer, 0, sizeof(buffer));

        // Read data from the client socket
        ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytesRead == -1)
        {
            cerr << "Failed to read data from client socket" << endl;
            close(client_fd);
            exit(1);
        }
        else if (bytesRead == 0)
        {
            // Client closed the connection
            cout << "Client disconnected" << endl;
            close(client_fd);
            return -1;
        }

        cout << "Received data from client: " << buffer << endl;
        return 0;

    };
}

int main()
{
    // Load the st_reactor.so library
    void *reactorLibHandle = dlopen("./st_reactor.so", RTLD_NOW);
    if (!reactorLibHandle)
    {
        cerr << "Failed to load st_reactor.so: " << dlerror() << endl;
        return 1;
    }

    // Function pointers to the library functions
    using CreateReactorFunc = void* (*)();
    using AddFd = void (*)(void*, int, std::function<int(int)>);
    using StartReactorFunc = void (*)(void*);
    using WaitForFunc = void (*)(void*);
    using StopReactorFunc = void (*)(void*);   


    // Retrieve function pointers from the library
    CreateReactorFunc createReactorFunc = (CreateReactorFunc)dlsym(reactorLibHandle, "createReactor");
    AddFd addFd = (AddFd)dlsym(reactorLibHandle, "addFd");
    StartReactorFunc startReactorFunc = (StartReactorFunc)dlsym(reactorLibHandle, "startReactor");
    WaitForFunc waitForFunc = (WaitForFunc)dlsym(reactorLibHandle, "waitFor");
    StopReactorFunc stopReactorFunc = (StopReactorFunc)dlsym(reactorLibHandle, "stopReactor");

    // Create the Reactor
    void *reactor = createReactorFunc();
    if (!reactor)
    {
        cerr << "Failed to create Reactor" << endl;
        dlclose(reactorLibHandle);
        return 1;
    }
    // Add the server socket FD to the Reactor
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        cerr << "Failed to create server socket" << endl;
        return 1;
    }
    //  Set up server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT); // Replace with your desired port number

    // Bind the socket to the server address
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Failed to bind server socket" << endl;
        close(serverSocket);
        return 1;
    }

    // Listen for client connections
    if (listen(serverSocket, SOMAXCONN) == -1)
    {
        cerr << "Failed to listen on server socket" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "Server started. Listening for connections..." << endl;
    // Start the Reactor
    startReactorFunc(reactor);
    
    // Accept and handle client connections
    while (true)
    {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
        if (clientSocket == -1)
        {
            cerr << "Failed to accept client connection" << endl;
            continue;
        }

        // Convert client IP address to string format
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIp, INET_ADDRSTRLEN);
        cout << "Accepted connection from: " << clientIp << endl;

        // Add the client socket FD to the Reactor
        addFd(reactor, clientSocket, handleClientData(clientSocket));
    }
    // Stop the Reactor
    stopReactorFunc(reactor);

    // Wait for the Reactor thread to finish
    waitForFunc(reactor);

    // Cleanup
    dlclose(reactorLibHandle);
    return 0;
}
