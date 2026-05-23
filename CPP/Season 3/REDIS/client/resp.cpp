#include "resp.hpp"
#include "connection.hpp"

#include <cstdlib>
#include <string>

std::string resp_encode(const std::vector<std::string> &args) {
    std::string result = "*" + std::to_string(args.size()) + "\r\n";
    for (const auto &arg : args) {
        result += "$" + std::to_string(arg.size()) + "\r\n";
        result += arg + "\r\n";
    }
    return result;
}

static std::optional<std::string> parse_bulk(ReaderBuf &reader) {
    auto len_line = reader.readLine();
    if (!len_line) return std::nullopt;
    long len = std::strtol(len_line->c_str(), nullptr, 10);
    if (len < 0) return std::make_optional<std::string>("(nil)");
    std::string value;
    if (!reader.readBytes((size_t)len, value)) return std::nullopt;
    if (!reader.consumeCRLF()) return std::nullopt;
    return '"' + value + '"';
}

std::optional<std::string> resp_parse(ReaderBuf &reader, int depth) {
    if (depth > 16) {
        return std::make_optional<std::string>("(error) RESP parsing depth exceeded");
    }

    auto prefix = reader.readByte();
    if (!prefix) return std::nullopt;

    switch (*prefix) {
        case '+': {
            auto line = reader.readLine();
            return line ? *line : std::nullopt;
        }
        case '-': {
            auto line = reader.readLine();
            return line ? std::string("(error) ") + *line : std::nullopt;
        }
        case ':': {
            auto line = reader.readLine();
            return line ? std::string("(integer) ") + *line : std::nullopt;
        }
        case '$':
            return parse_bulk(reader);
        case '*': {
            auto line = reader.readLine();
            if (!line) return std::nullopt;
            long count = std::strtol(line->c_str(), nullptr, 10);
            if (count < 0) return std::make_optional<std::string>("(nil)");
            if (count == 0) return std::make_optional<std::string>("(empty array)");
            std::string result;
            for (long i = 0; i < count; i++) {
                auto item = resp_parse(reader, depth + 1);
                if (!item) return std::nullopt;
                result += std::to_string(i + 1) + ") " + *item;
                if (i + 1 < count) result += "\n";
            }
            return result;
        }
        default:
            return std::make_optional<std::string>("(error) Unknown RESP type: " + std::string(1, *prefix));
    }
}
