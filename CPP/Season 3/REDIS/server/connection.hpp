#pragma once

#include <optional>
#include <string>

#include "resp.hpp"

class Connection {
public:
    explicit Connection(int fd);
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    ~Connection();

    bool readMore();
    std::optional<RespValue> nextRequest();
    bool sendResponse(const RespValue& response);

private:
    int fd_;
    std::string buffer_;
};
