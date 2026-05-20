#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

static std::string resp_encode(const std::vector<std::string> &args) {
    std::string result = "*" + std::to_string(args.size()) + "\r\n";
    for (const auto &arg : args)
        result += "$" + std::to_string(arg.size()) + "\r\n" + arg + "\r\n";
    return result;
}

class ReaderBuf {
public:
    explicit ReaderBuf(int fd) : fd_(fd) {}

    char read_byte() {
        char byte;
        ssize_t n = ::read(fd_, &byte, 1);
        if (n < 0) { std::cerr << "Error reading from socket\n"; return -1; }
        if (n == 0) { std::cerr << "Connection closed by peer\n"; return -1; }
        return byte;
    }

    std::string read_line() {
        std::string line;
        char byte;
        while ((byte = read_byte()) != -1) {
            if (byte == '\r') { read_byte(); break; }
            line += byte;
        }
        return line;
    }

    std::string read_bytes(long n) {
        std::string data(n, '\0');
        long total = 0;
        while (total < n) {
            ssize_t count = ::read(fd_, &data[total], n - total);
            if (count < 0) { std::cerr << "Error reading from socket\n"; return ""; }
            if (count == 0) { std::cerr << "Connection closed by peer\n"; return ""; }
            total += count;
        }
        read_byte();
        read_byte();
        return data;
    }

private:
    int fd_;
};

static std::string resp_parse(ReaderBuf &r, int depth = 0) {
    if (depth > 10) { std::cerr << "RESP parsing depth exceeded\n"; return ""; }
    char prefix = r.read_byte();
    if (prefix == -1) return "";
    switch (prefix) {
        case '+': return r.read_line();
        case '-': return "(error) " + r.read_line();
        case ':': return "(integer) " + r.read_line();
        case '$': {
            long len = std::stol(r.read_line());
            if (len < 0) return "(nil)";
            return '"' + r.read_bytes(len) + '"';
        }
        case '*': {
            long count = std::stol(r.read_line());
            if (count < 0) return "(nil)";
            if (count == 0) return "(empty array)";
            std::string result;
            for (long i = 0; i < count; i++) {
                result += std::to_string(i + 1) + ") " + resp_parse(r, depth + 1);
                if (i < count - 1) result += "\n";
            }
            return result;
        }
        default:
            std::cerr << "Unknown RESP type: " << prefix << "\n";
            return "";
    }
}

static std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string cur;
    bool in_single = false, in_double = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == ' ' && !in_single && !in_double) {
            if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
        } else if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) tokens.push_back(cur);
    return tokens;
}

static int tcp_connect(const std::string &host, const std::string &port) {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = nullptr;
    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (rc != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rc) << "\n";
        return -1;
    }
    int fd = -1;
    for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0)
        std::cerr << "Unable to connect to " << host << ":" << port << "\n";
    return fd;
}

static void send_command(int fd, ReaderBuf &reader, const std::vector<std::string> &args) {
    if (args.empty()) return;
    std::string cmd = resp_encode(args);
    if (write(fd, cmd.c_str(), cmd.size()) < 0) {
        std::cerr << "Error writing to socket\n";
        return;
    }
    std::cout << resp_parse(reader) << '\n';
}

static void run_repl(int fd, ReaderBuf &reader, const std::string &host, const std::string &port) {
    std::string prompt = host + ":" + port + "> ";
    std::string line;
    while (true) {
        std::cout << prompt << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << '\n'; break; }
        auto args = tokenize(line);
        if (args.empty()) continue;
        if (args[0] == "exit" || args[0] == "quit") break;
        send_command(fd, reader, args);
    }
}

int main(int argc, char **argv) {
    std::string host = "127.0.0.1";
    std::string port = "6379";
    std::vector<std::string> cmd_args;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if      (a == "-h" && i + 1 < argc) host = argv[++i];
        else if (a == "-p" && i + 1 < argc) port = argv[++i];
        else cmd_args.push_back(a);
    }

    int fd = tcp_connect(host, port);
    if (fd < 0) return 1;

    ReaderBuf reader(fd);

    if (cmd_args.empty())
        run_repl(fd, reader, host, port);
    else
        send_command(fd, reader, cmd_args);

    close(fd);
    return 0;
}