#include "connection.hpp"
#include "resp.hpp"

#include <iostream>
#include <string>
#include <vector>

static std::vector<std::string> tokenize(const std::string &line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_single_quote = false;
    bool in_double_quote = false;

    for (char c : line) {
        if (c == ' ' && !in_single_quote && !in_double_quote) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else if (c == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (c == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else {
            current.push_back(c);
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

static bool send_command(Connection &conn, const std::vector<std::string> &args) {
    if (args.empty()) {
        return true;
    }

    const std::string payload = resp_encode(args);

    if (!conn.send(payload)) {
        std::cerr << "Failed to send command to "
                  << conn.host() << ':' << conn.port() << '\n';
        return false;
    }

    const auto response = conn.receive();
    if (!response) {
        std::cerr << "Server disconnected or sent malformed response\n";
        return false;
    }

    std::cout << *response << '\n';
    return true;
}

static void run_repl(Connection &conn) {
    const std::string prompt = conn.host() + ":" + conn.port() + "> ";
    std::string line;

    while (conn.connected()) {
        std::cout << prompt << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        const auto args = tokenize(line);

        if (args.empty()) {
            continue;
        }

        if (args[0] == "exit" || args[0] == "quit") {
            break;
        }

        if (!send_command(conn, args)) {
            break;
        }
    }
}

int main(int argc, char **argv) {
    std::string host = "127.0.0.1";
    std::string port = "6379";
    std::vector<std::string> command_args;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            port = argv[++i];
        } else {
            command_args.push_back(arg);
        }
    }

    Connection conn(host, port);

    if (!conn.connect()) {
        return 1;
    }

    bool success = false;

    if (command_args.empty()) {
        run_repl(conn);
        success = conn.connected();
    } else {
        success = send_command(conn, command_args);
    }

    conn.close();
    return success ? 0 : 1;
}
