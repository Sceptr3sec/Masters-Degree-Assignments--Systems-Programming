#pragma once

#include <optional>
#include <string>
#include <vector>

struct RespValue {
    enum class Type { SimpleString, Error, Integer, BulkString, Array, Null };

    Type type = Type::Null;
    std::string str;
    long long integer = 0;
    std::vector<RespValue> array;

    static RespValue simple(const std::string& s);
    static RespValue error(const std::string& s);
    static RespValue integer_val(long long i);
    static RespValue bulk(const std::string& s);
    static RespValue null_val();
    static RespValue array_val(std::vector<RespValue> a);
};

std::string serialize(const RespValue& val);
std::optional<RespValue> parseResp(const std::string& buf, size_t& pos);
