#include "connection.hpp"
#include "resp.hpp"

#include <iostream>
#include <string>
#include <vector>


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


static bool send_command(Connection &conn, const std::vector<std::string> &args) {
    if (args.empty()) return true;
    std::string payload = resp_encode(args);
    if (!conn.send(payload)) {
        std::cerr << "Failed to send command to " << conn.host() << ':' << conn.port() << "\n";
        return false;
    }

    auto response = conn.receive();
    if (!response) {
        std::cerr << "Server disconnected or sent malformed response\n";
        return false;
    }

    std::cout << *response << '\n';
    return true;
}

static void run_repl(Connection &conn) {
    std::string prompt = conn.host() + ":" + conn.port() + "> ";
    std::string line;
    while (conn.connected()) {
        std::cout << prompt << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }
        auto args = tokenize(line);
        if (args.empty()) continue;
        if (args[0] == "exit" || args[0] == "quit") break;
        if (!send_command(conn, args)) break;
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

    Connection conn(host, port);
    if (!conn.connect()) return 1;

    bool success;
    if (cmd_args.empty()) {
        run_repl(conn);
        success = conn.connected();
    } else {
        success = send_command(conn, cmd_args);
    }

    conn.close();
    return success ? 0 : 1;
}