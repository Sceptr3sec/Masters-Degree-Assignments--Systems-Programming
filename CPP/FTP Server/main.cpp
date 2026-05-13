#include <iostream>
#include <string>
#include <cctype>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <limits.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>

std::string resolvePath(const std::string &root, const std::string &cwd, const std::string &path) {
    std::string combined;
    if (!path.empty() && path[0] == '/') {
        combined = root + path;
    } else {
        combined = root + cwd + "/" + path;
    }
    char resolved[PATH_MAX];
    if (realpath(combined.c_str(), resolved) == nullptr) return "";
    std::string resolvedStr(resolved);
    if (resolvedStr.find(root) != 0) return "";
    return resolvedStr;
}

//Opens data connection in either active or passive mode, returns data socket fd or -1 on error
int openDataConnection(bool isPasv, int &pasvFd, bool isPort, sockaddr_in &portaddr) {
    if(isPasv && pasvFd >= 0) {
        int dataFd = accept(pasvFd, nullptr, nullptr);
        close(pasvFd); pasvFd = -1;
        return dataFd;
    } else if(isPort) {
        int dataFd = socket(AF_INET, SOCK_STREAM, 0);
        if(dataFd < 0) return -1;
        if(connect(dataFd, reinterpret_cast<sockaddr*>(&portaddr), sizeof(portaddr)) < 0) {
            close(dataFd);
            return -1;
        }
        return dataFd;
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <root_directory>\n";
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string rootDir = argv[2];
    std::string cwd = "/";
    int pasvFd = -1;
    bool isPasv = false;
    sockaddr_in portAddr{};
    bool isPort = false;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    listen(serverFd, 5);
    std::cout << "FTP Server running on port " << port << "\n";
    std::cout << "Root directory: " << rootDir << "\n";

    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
    std::cout << "Client connected: " << ip << "\n";

    std::string greeting = "220 Welcome to Simple FTP Server\r\n";
    write(clientFd, greeting.c_str(), greeting.size());

    std::string line;
    char c;

    while (true) {
        int n = read(clientFd, &c, 1);
        if (n <= 0) { std::cout << "Client disconnected.\n"; break; }

        if (c == '\n') {
            if (!line.empty() && line.back() == '\r') line.pop_back();

            size_t spacePos = line.find(' ');
            std::string cmd = line.substr(0, spacePos);
            for (char &ch : cmd) ch = toupper(ch);
            line = (spacePos != std::string::npos) ? cmd + line.substr(spacePos) : cmd;

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

            } else if (line == "PWD" || line == "XPWD") {
                std::string reply = "257 \"" + cwd + "\" is the current directory\r\n";
                write(clientFd, reply.c_str(), reply.size());

            } else if (line.substr(0, 3) == "CWD") {
                std::string path = line.substr(4);
                std::string newPath = resolvePath(rootDir, cwd, path);
                if (!newPath.empty()) {
                    cwd = newPath.substr(rootDir.size());
                    if (cwd.empty()) cwd = "/";
                    std::string reply = "250 Directory changed to " + cwd + "\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                } else {
                    std::string reply = "550 Failed to change directory\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                }

            } else if (line == "CDUP") {
                size_t lastSlash = cwd.rfind('/');
                if (lastSlash == 0 || lastSlash == std::string::npos) {
                    cwd = "/";
                } else {
                    cwd = cwd.substr(0, lastSlash);
                }
                std::string reply = "250 Directory changed to " + cwd + "\r\n";
                write(clientFd, reply.c_str(), reply.size());

            } else if (line == "PASV") {
                if (pasvFd != -1) { close(pasvFd); pasvFd = -1; }

                pasvFd = socket(AF_INET, SOCK_STREAM, 0);
                if (pasvFd < 0) { perror("socket"); return 1; }
                int o = 1;
                setsockopt(pasvFd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));

                sockaddr_in pasvAddr{};
                pasvAddr.sin_family      = AF_INET;
                pasvAddr.sin_addr.s_addr = INADDR_ANY;
                pasvAddr.sin_port        = htons(0);

                if (bind(pasvFd, reinterpret_cast<sockaddr*>(&pasvAddr), sizeof(pasvAddr)) < 0) {
                    perror("bind"); return 1;
                }

                sockaddr_in assigned{};
                socklen_t assignedLen = sizeof(assigned);
                getsockname(pasvFd, reinterpret_cast<sockaddr*>(&assigned), &assignedLen);
                listen(pasvFd, 1);

                int pasvPort = ntohs(assigned.sin_port);
                sockaddr_in localAddr{};
                socklen_t localLen = sizeof(localAddr);
                getsockname(clientFd, reinterpret_cast<sockaddr*>(&localAddr), &localLen);
                unsigned char *ip4 = reinterpret_cast<unsigned char*>(&localAddr.sin_addr.s_addr);

                char reply[128];
                snprintf(reply, sizeof(reply),
                    "227 Entering Passive Mode (%u,%u,%u,%u,%d,%d)\r\n",
                    ip4[0], ip4[1], ip4[2], ip4[3], pasvPort / 256, pasvPort % 256);
                write(clientFd, reply, strlen(reply));
                isPasv = true;

            } else if (line == "LIST" || line.substr(0, 5) == "LIST ") {
                if ((!isPasv || pasvFd < 0) && !isPort) {
                    std::string reply = "425 Use PASV first\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                } else {
                    std::string reply150 = "150 Here comes the directory listing\r\n";
                    write(clientFd, reply150.c_str(), reply150.size());

                    int dataFd = openDataConnection(isPasv, pasvFd, isPort, portAddr);
                    isPasv = false;
                    isPort = false;

                    if (dataFd >= 0) {
                        std::string dirPath = resolvePath(rootDir, cwd, "");
                        DIR *dir = opendir(dirPath.c_str());
                        if (dir) {
                            struct dirent *entry;
                            while ((entry = readdir(dir)) != nullptr) {
                                std::string name = entry->d_name;
                                if (name == "." || name == "..") continue;
                                std::string entryPath = dirPath + "/" + name;
                                struct stat st{};
                                if (stat(entryPath.c_str(), &st) != 0) continue;

                                char perms[11];
                                perms[0]  = S_ISDIR(st.st_mode) ? 'd' : '-';
                                perms[1]  = (st.st_mode & S_IRUSR) ? 'r' : '-';
                                perms[2]  = (st.st_mode & S_IWUSR) ? 'w' : '-';
                                perms[3]  = (st.st_mode & S_IXUSR) ? 'x' : '-';
                                perms[4]  = (st.st_mode & S_IRGRP) ? 'r' : '-';
                                perms[5]  = (st.st_mode & S_IWGRP) ? 'w' : '-';
                                perms[6]  = (st.st_mode & S_IXGRP) ? 'x' : '-';
                                perms[7]  = (st.st_mode & S_IROTH) ? 'r' : '-';
                                perms[8]  = (st.st_mode & S_IWOTH) ? 'w' : '-';
                                perms[9]  = (st.st_mode & S_IXOTH) ? 'x' : '-';
                                perms[10] = '\0';

                                char timebuf[16];
                                struct tm *tm = localtime(&st.st_mtime);
                                strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm);

                                char fileline[512];
                                snprintf(fileline, sizeof(fileline),
                                    "%s %3lu ftp ftp %8lu %s %s\r\n",
                                    perms, (unsigned long)st.st_nlink,
                                    (unsigned long)st.st_size, timebuf, name.c_str());
                                write(dataFd, fileline, strlen(fileline));
                            }
                            closedir(dir);
                        }
                        close(dataFd);
                    }
                    std::string reply226 = "226 Directory send OK\r\n";
                    write(clientFd, reply226.c_str(), reply226.size());
                }

            } else if (line.substr(0, 4) == "RETR") {
                if ((!isPasv || pasvFd < 0) && !isPort) {
                    std::string reply = "425 Use PASV first\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                } else {
                    std::string filename = line.substr(5);
                    std::string filePath = resolvePath(rootDir, cwd, filename);
                    if (filePath.empty()) {
                        std::string reply = "550 File not found\r\n";
                        write(clientFd, reply.c_str(), reply.size());
                        close(pasvFd); pasvFd = -1; isPasv = false;
                    } else {
                        FILE *file = fopen(filePath.c_str(), "rb");
                        if (!file) {
                            std::string reply = "550 Cannot open file\r\n";
                            write(clientFd, reply.c_str(), reply.size());
                            close(pasvFd); pasvFd = -1; isPasv = false;
                        } else {
                            std::string reply150 = "150 Opening data connection\r\n";
                            write(clientFd, reply150.c_str(), reply150.size());

                            int dataFd = openDataConnection(isPasv, pasvFd, isPort, portAddr);
                            isPasv = false;
                            isPort = false;

                            if (dataFd >= 0) {
                                char buffer[65536];
                                size_t bytesRead;
                                while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                                    ssize_t sent = 0;
                                    while (sent < (ssize_t)bytesRead) {
                                        ssize_t w = write(dataFd, buffer + sent, bytesRead - sent);
                                        if (w <= 0) break;
                                        sent += w;
                                    }
                                }
                                close(dataFd);
                            }
                            fclose(file);
                            std::string reply226 = "226 Transfer complete\r\n";
                            write(clientFd, reply226.c_str(), reply226.size());
                        }
                    }
                }

            } else if (line.substr(0, 4) == "STOR") {
                if ((!isPasv || pasvFd < 0) && !isPort) {
                    std::string reply = "425 Use PASV first\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                } else {
                    std::string filename = line.substr(5);
                    std::string fullPath;
                    bool pathOk = true;

                    size_t lastSlash = filename.rfind('/');
                    if (lastSlash != std::string::npos) {
                        std::string dir         = filename.substr(0, lastSlash);
                        std::string name        = filename.substr(lastSlash + 1);
                        std::string resolvedDir = resolvePath(rootDir, cwd, dir);
                        if (resolvedDir.empty()) {
                            std::string reply = "553 Directory not found\r\n";
                            write(clientFd, reply.c_str(), reply.size());
                            close(pasvFd); pasvFd = -1; isPasv = false;
                            pathOk = false;
                        } else {
                            fullPath = resolvedDir + "/" + name;
                        }
                    } else {
                        std::string resolvedDir = resolvePath(rootDir, cwd, "");
                        if (resolvedDir.empty()) {
                            std::string reply = "553 Cannot resolve directory\r\n";
                            write(clientFd, reply.c_str(), reply.size());
                            close(pasvFd); pasvFd = -1; isPasv = false;
                            pathOk = false;
                        } else {
                            fullPath = resolvedDir + "/" + filename;
                        }
                    }

                    if (pathOk) {
                        FILE *file = fopen(fullPath.c_str(), "wb");
                        if (!file) {
                            std::string reply = "553 Cannot create file\r\n";
                            write(clientFd, reply.c_str(), reply.size());
                            close(pasvFd); pasvFd = -1; isPasv = false;
                        } else {
                            std::string reply150 = "150 Opening data connection\r\n";
                            write(clientFd, reply150.c_str(), reply150.size());

                            int dataFd = openDataConnection(isPasv, pasvFd, isPort, portAddr);
                            isPasv = false;
                            isPort = false;

                            if (dataFd >= 0) {
                                char buffer[65536];
                                ssize_t bytesRead;
                                while ((bytesRead = read(dataFd, buffer, sizeof(buffer))) > 0) {
                                    fwrite(buffer, 1, bytesRead, file);
                                }
                                close(dataFd);
                            }
                            fclose(file);
                            std::string reply226 = "226 Transfer complete\r\n";
                            write(clientFd, reply226.c_str(), reply226.size());
                        }
                    }
                }

            } else if (line.substr(0, 4) == "PORT") {
                std::string arg = line.substr(5);

                unsigned int h1, h2, h3, h4, p1, p2;
                if(sscanf(arg.c_str(), "%u,%u,%u,%u,%u,%u", &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
                    std::string reply = "501 syntax error in parameters\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                } else {
                    //Build IP string
                    char ipStr[16];
                    snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", h1, h2, h3, h4);
                    portAddr = {};
                    portAddr.sin_family = AF_INET;
                    portAddr.sin_port = htons(p1 * 256 + p2);
                    inet_pton(AF_INET, ipStr, &portAddr.sin_addr);
                // Cancen any pending passive mode
                    if(pasvFd >= 0) {close(pasvFd); pasvFd = -1; isPasv = false;}
                    isPasv = false;
                    isPort = true;

                    std::string reply = "200 PORT command successful\r\n";
                    write(clientFd, reply.c_str(), reply.size());
                }
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
