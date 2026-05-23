#include "connection.hpp"

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

namespace {
static constexpr size_t READ_BUFFER_SIZE = 65536;
}

Connection::Connection(int fd) : fd_(fd) {}

Connection::~Connection() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool Connection::readMore() {
    char tmp[READ_BUFFER_SIZE];
    while (true) {
        ssize_t n = recv(fd_, tmp, sizeof(tmp), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        buffer_.append(tmp, static_cast<size_t>(n));
        return true;
    }
}

std::optional<RespValue> Connection::nextRequest() {
    size_t pos = 0;
    try {
        auto resp = parseResp(buffer_, pos);
        if (!resp) return std::nullopt;
        buffer_.erase(0, pos);
        return resp;
    } catch (const std::runtime_error&) {
        return std::nullopt;
    }
}

bool Connection::sendResponse(const RespValue& response) {
    std::string reply = serialize(response);
    size_t sent = 0;
    while (sent < reply.size()) {
        ssize_t n = ::send(fd_, reply.data() + sent, reply.size() - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}
