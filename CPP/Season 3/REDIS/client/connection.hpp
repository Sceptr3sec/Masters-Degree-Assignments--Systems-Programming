#pragma once

#include <array>
#include <optional>
#include <string>

class ReaderBuf {
public:
    explicit ReaderBuf(int fd = -1);
    void reset(int fd);
    bool connected() const;
    std::optional<char> readByte();
    std::optional<std::string> readLine();
    bool readBytes(size_t count, std::string &out);
    bool consumeCRLF();

private:
    bool refill();

    int fd_;
    bool eof_;
    std::array<char, 4096> buf_;
    size_t pos_;
    size_t end_;
};

class Connection {
public:
    Connection(std::string host, std::string port);
    ~Connection();

    bool connect();
    void close();
    bool connected() const;
    bool send(const std::string &payload);
    std::optional<std::string> receive();

    const std::string &host() const;
    const std::string &port() const;

private:
    std::string host_;
    std::string port_;
    int fd_;
    ReaderBuf reader_;
};
