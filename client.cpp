#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    // connect to the server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    // send a message to the server
    const char* message = "Hello from client";
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("send");
        exit(1);
    }

    // receive the response from the server
    char buffer[1024];
    int num_bytes = recv(sock, buffer, sizeof(buffer), 0);
    if (num_bytes < 0) {
        perror("recv");
        exit(1);
    }

    // print the response
    std::cout << "Server response: " << buffer << std::endl;

    // close the socket
    close(sock);

    return 0;
}
