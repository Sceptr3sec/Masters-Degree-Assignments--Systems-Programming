#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>

int main() {
    // Create a socket
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return 1; }

    // Allow port reuse
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to address and port
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(2121);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(serverFd, 5);
    std::cout << "FTP Server running on port 2121...\n";

    // Accept one connection
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd = accept(serverFd,
                          reinterpret_cast<sockaddr*>(&clientAddr),
                          &clientAddrLen);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
    std::cout << "Client connected: " << ip << "\n";

    // Send greeting
    std::string greeting = "220 Welcome to Simple FTP Server\r\n";
    write(clientFd, greeting.c_str(), greeting.size());

    std::string line;
    char c;

    while (true) {
        int n = read(clientFd, &c, 1);
        if (n <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        if (c == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            std::cout << "Received: " << line << "\n";

            if (line.substr(0, 4) == "USER") {
                std::string reply = "331 Username OK, need password\r\n";
                write(clientFd, reply.c_str(), reply.size());
            } else if (line.substr(0, 4) == "PASS") {
                std::string reply = "230 User logged in\r\n";
                write(clientFd, reply.c_str(), reply.size());
            } else if (line == "QUIT") {
                std::string reply = "221 Goodbye\r\n";
                write(clientFd, reply.c_str(), reply.size());
                break;
            } else {
                std::string reply = "500 Unknown command\r\n";
                write(clientFd, reply.c_str(), reply.size());
            }

            line.clear();
        } else {
            line += c;
        }
    }

    sleep(3);
    close(clientFd);
    close(serverFd);
    return 0;
}
