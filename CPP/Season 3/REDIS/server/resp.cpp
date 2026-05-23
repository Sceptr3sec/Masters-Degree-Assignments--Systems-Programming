#include "resp.hpp"

#include <cctype>
#include <stdexcept>
#include <sstream>

namespace {

static std::string readLine(const std::string& buf, size_t& pos) {
    size_t start = pos;
    while (pos + 1 < buf.size()) {
        if (buf[pos] == '\r' && buf[pos + 1] == '\n') {
            std::string line = buf.substr(start, pos - start);
            pos += 2;
            return line;
        }
        pos++;
    }
    throw std::runtime_error("Incomplete RESP data");
}

static std::optional<RespValue> parseBulk(const std::string& buf, size_t& pos) {
    long long len = std::stoll(readLine(buf, pos));
    if (len == -1) return RespValue::null_val();
    if ((long long)(buf.size() - pos) < len + 2) throw std::runtime_error("Incomplete RESP data");
    std::string data = buf.substr(pos, len);
    pos += len + 2; // consume CRLF
    return RespValue::bulk(data);
}

static std::optional<RespValue> parseArray(const std::string& buf, size_t& pos) {
    long long count = std::stoll(readLine(buf, pos));
    if (count == -1) return RespValue::null_val();
    std::vector<RespValue> elems;
    for (long long i = 0; i < count; i++) {
        auto e = parseResp(buf, pos);
        if (!e) throw std::runtime_error("Incomplete RESP data");
        elems.push_back(std::move(*e));
    }
    return RespValue::array_val(std::move(elems));
}

static std::optional<RespValue> parseInline(const std::string& buf, size_t& pos) {
    std::string line = readLine(buf, pos);
    std::vector<RespValue> parts;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) parts.push_back(RespValue::bulk(tok));
    if (parts.empty()) return std::nullopt;
    return RespValue::array_val(std::move(parts));
}

} // namespace

RespValue RespValue::simple(const std::string& s) {
    RespValue v;
    v.type = Type::SimpleString;
    v.str = s;
    return v;
}

RespValue RespValue::error(const std::string& s) {
    RespValue v;
    v.type = Type::Error;
    v.str = s;
    return v;
}

RespValue RespValue::integer_val(long long i) {
    RespValue v;
    v.type = Type::Integer;
    v.integer = i;
    return v;
}

RespValue RespValue::bulk(const std::string& s) {
    RespValue v;
    v.type = Type::BulkString;
    v.str = s;
    return v;
}

RespValue RespValue::null_val() {
    RespValue v;
    v.type = Type::Null;
    return v;
}

RespValue RespValue::array_val(std::vector<RespValue> a) {
    RespValue v;
    v.type = Type::Array;
    v.array = std::move(a);
    return v;
}

std::string serialize(const RespValue& val) {
    switch (val.type) {
        case RespValue::Type::SimpleString:
            return "+" + val.str + "\r\n";
        case RespValue::Type::Error:
            return "-" + val.str + "\r\n";
        case RespValue::Type::Integer:
            return ":" + std::to_string(val.integer) + "\r\n";
        case RespValue::Type::BulkString:
            return "$" + std::to_string(val.str.size()) + "\r\n" + val.str + "\r\n";
        case RespValue::Type::Null:
            return "$-1\r\n";
        case RespValue::Type::Array: {
            std::string out = "*" + std::to_string(val.array.size()) + "\r\n";
            for (const auto& e : val.array) out += serialize(e);
            return out;
        }
    }
    return "-ERR internal\r\n";
}

std::optional<RespValue> parseResp(const std::string& buf, size_t& pos) {
    if (pos >= buf.size()) return std::nullopt;
    char t = buf[pos++];
    switch (t) {
        case '+': return RespValue::simple(readLine(buf, pos));
        case '-': return RespValue::error(readLine(buf, pos));
        case ':': return RespValue::integer_val(std::stoll(readLine(buf, pos)));
        case '$': return parseBulk(buf, pos);
        case '*': return parseArray(buf, pos);
        default:
            pos--;
            return parseInline(buf, pos);
    }
}
