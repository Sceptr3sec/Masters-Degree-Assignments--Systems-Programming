#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static std::string resp_encode(const std::vector<std::string> &args) {
    std::string result = "*" + std::to_string(args.size()) + "\r\n";
    for (const auto &arg : args) {
        result += "$" + std::to_string(arg.size()) + "\r\n" + arg + "\r\n";
    }
    return result;
}

class ReaderBuf{
    public:
        ReaderBuf(int fd) : fd(fd) {}
}

char read_byte() {
    char byte;
    ssize_t n = read(fd, &byte, 1);
    if (n < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        return -1;
    } else if (n == 0) {
        std::cerr << "Connection closed by peer" << std::endl;
        return -1;
    }
    return byte;
}

std::string read_line(){
    std::string line;
    char byte;
    while ((byte = read_byte()) != -1) {
        if (byte == '\r') {
            // Expecting '\n' after '\r'
            if (read_byte() == '\n') {
                break; // End of line
            } else {
                std::cerr << "Malformed line ending" << std::endl;
                return "";
            }
        }
        line += byte;
    }
    return line;
}

std::string read_bytes(long n){
    std::string data(n, '\0');
    long total = 0;
    while (total < n) {
        ssize_t count = read(fd, &data[total], n - total);
        if (count < 0) {
            std::cerr << "Error reading from socket" << std::endl;
            return "";
        } else if (count == 0) {
            std::cerr << "Connection closed by peer" << std::endl;
            return "";
        }
        total += count;
    }
    read_byte(); // Consume the trailing '\r'
    read_byte(); // Consume the trailing '\n'
    return data;
}

static std::string resp_parse(ReaderBuf &r, int depth = 0) {
    if (depth > 10) {
        std::cerr << "RESP parsing depth exceeded" << std::endl;
        return "";
    }

    char prefix = r.read_byte();
    if (prefix == -1) {
        return "";
    }

    switch (prefix) {
        case '+': // Simple String
            return r.read_line();
        case '-': // Error
            std::cerr << "Redis error: " << r.read_line() << std::endl;
            return "";
        case ':': // Integer
            return r.read_line();
        case '$': { // Bulk String
            long len = std::stol(r.read_line());
            if (len < 0) return ""; // Null bulk string
            return r.read_bytes(len);
        }
        case '*': { // Array
            long count = std::stol(r.read_line());
            std::string result = "[";
            for (long i = 0; i < count; i++) {
                result += resp_parse(r, depth + 1);
                if (i < count - 1) result += ", ";
            }
            result += "]";
            return result;
        }
        default:
            std::cerr << "Unknown RESP type: " << prefix << std::endl;
            return "";
    }
}

static std::vector<std::string> tokenize(const std::string &line){
    std::vector<std::string> tokens:
    std::string cur;
    bool in_single = false, in_double = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == ' ' && !in_single && !in_double) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) {
        tokens.push_back(cur);
    }
    return tokens;
}

static int tcp_connect(const std::string &host, const std::string &port) {
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (rc != 0)
        std::cerr << "getaddrinfo: " << gai_strerror(rc) << std::endl;

    int fd = -1;
    for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;

        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;

    if(fd < 0)
        std::cerr << "Unable to connect to " << host << ":" << port << std::endl;
}