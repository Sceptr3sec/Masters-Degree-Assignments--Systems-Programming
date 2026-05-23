#pragma once

#include <optional>
#include <string>
#include <vector>

class ReaderBuf;

std::string resp_encode(const std::vector<std::string> &args);
std::optional<std::string> resp_parse(ReaderBuf &reader, int depth = 0);
