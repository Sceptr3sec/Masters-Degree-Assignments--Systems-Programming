#include "connection.hpp"
#include "resp.hpp"

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

ReaderBuf::ReaderBuf(int fd)
    : fd_(fd), eof_(false), pos_(0), end_(0) {}

void ReaderBuf::reset(int fd) {
    fd_ = fd;
    eof_ = false;
    pos_ = 0;
    end_ = 0;
}

bool ReaderBuf::connected() const {
    return fd_ >= 0 && !eof_;
}

bool ReaderBuf::refill() {
    if (fd_ < 0 || eof_) return false;
    ssize_t n = ::read(fd_, buf_.data(), buf_.size());
    if (n < 0) {
        std::cerr << "Error reading from socket\n";
        eof_ = true;
        return false;
    }
    if (n == 0) {
        eof_ = true;
        return false;
    }
    pos_ = 0;
    end_ = (size_t)n;
    return true;
}

std::optional<char> ReaderBuf::readByte() {
    if (pos_ >= end_) {
        if (!refill()) return std::nullopt;
    }
    return buf_[pos_++];
}

std::optional<std::string> ReaderBuf::readLine() {
    std::string result;
    while (true) {
        auto ch = readByte();
        if (!ch) return std::nullopt;
        if (*ch == '\r') {
            auto next = readByte();
            if (!next) return std::nullopt;
            if (*next != '\n') {
                std::cerr << "Malformed RESP line ending\n";
            }
            break;
        }
        if (*ch == '\n') {
            break;
        }
        result.push_back(*ch);
    }
    return result;
}

bool ReaderBuf::readBytes(size_t count, std::string &out) {
    out.clear();
    out.reserve(count);
    while (out.size() < count) {
        if (pos_ >= end_) {
            if (!refill()) return false;
        }
        size_t available = std::min(count - out.size(), end_ - pos_);
        out.append(buf_.data() + pos_, available);
        pos_ += available;
    }
    return true;
}

bool ReaderBuf::consumeCRLF() {
    auto first = readByte();
    if (!first || *first != '\r') return false;
    auto second = readByte();
    if (!second || *second != '\n') return false;
    return true;
}

Connection::Connection(std::string host, std::string port)
    : host_(std::move(host)), port_(std::move(port)), fd_(-1), reader_(-1) {}

Connection::~Connection() {
    close();
}

bool Connection::connect() {
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result = nullptr;
    int rc = getaddrinfo(host_.c_str(), port_.c_str(), &hints, &result);
    if (rc != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rc) << "\n";
        return false;
    }

    for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
        fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd_ < 0) continue;
        if (::connect(fd_, rp->ai_addr, rp->ai_addrlen) == 0) break;
        ::close(fd_);
        fd_ = -1;
    }
    freeaddrinfo(result);
    if (fd_ < 0) {
        std::cerr << "Unable to connect to " << host_ << ":" << port_ << "\n";
        return false;
    }
    reader_.reset(fd_);
    return true;
}

void Connection::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    reader_.reset(-1);
}

bool Connection::connected() const {
    return fd_ >= 0 && reader_.connected();
}

bool Connection::send(const std::string &payload) {
    size_t total = 0;
    while (total < payload.size()) {
        ssize_t n = ::write(fd_, payload.data() + total, payload.size() - total);
        if (n < 0) {
            std::cerr << "Error writing to socket\n";
            return false;
        }
        total += (size_t)n;
    }
    return true;
}

std::optional<std::string> Connection::receive() {
    return resp_parse(reader_);
}

const std::string &Connection::host() const {
    return host_;
}

const std::string &Connection::port() const {
    return port_;
}
